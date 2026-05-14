#include "pch.h"
#include "StreamedMsixFile.h"

#include "Core/Compression/Zip/ZipLocalFileHeader.h"
#include "Core/Compression/Zip/ZipCentralDirectoryFileHeader.h"
#include "Core/Compression/Zip/ZipEndOfCentralDirectory.h"
#include "Core/Compression/Zip/ZipExtraFieldHeader.h"
#include "Core/Compression/Zip/ZipDataDescriptor.h"

#include "Core/Util/Memory.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Util/Scope.h"
#include "Core/Util/Guid.h"
#include "Core/IO/WinRTFileStream.h"
#include "Core/Net/Url.h"
#include "Core/Codec/Base64.h"
#include "Core/Unicode/Utf.h"
#include "Core/Coroutine/LazyTask.h"

#include <algorithm>
#include <optional>
#include <concepts>
#include <unordered_set>
#include <shared_mutex>

#include <winrt/Citrine.h>

#include <zlib-ng.h>

#include <pugixml.hpp>

#include <botan/hash.h>

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Storage::Streams;
}

using namespace Citrine;
using namespace Windows;

namespace {
	
	class BufferedInputStream {
	public:

		static constexpr auto StreamBufferCapacity = 0x10000;

		BufferedInputStream(winrt::IInputStream baseStream)

			: baseStream(std::move(baseStream))
			, streamBuffer(winrt::Buffer{ StreamBufferCapacity })
			, streamBufferPosition(StreamBufferCapacity)
		{
			streamBuffer.Length(StreamBufferCapacity);
		}

		BufferedInputStream(BufferedInputStream const&) = delete;
		auto operator=(BufferedInputStream const&) = delete;

		auto ReadAsync(winrt::IBuffer const& buffer, std::uint32_t count) -> LazyTask<> {

			if (buffer.Capacity() < count)
				throw winrt::hresult_invalid_argument{};

			auto streamBufferSize = streamBuffer.Length();
			while (bytesToSkip > 0 && streamBufferSize > 0) {

				auto newPos = streamBufferPosition + bytesToSkip;
				if (newPos <= streamBufferSize) {

					streamBufferPosition = newPos;
					bytesToSkip = 0;
					break;
				}

				streamBufferPosition = streamBufferSize;
				bytesToSkip = newPos - streamBufferSize;

				streamBuffer = co_await baseStream.ReadAsync(streamBuffer, StreamBufferCapacity, {});
				streamBufferPosition = 0;
				streamBufferSize = streamBuffer.Length();
			}

			auto out = buffer.data();
			auto bytesToRead = count;

			auto streamBufferData = streamBuffer.data();
			while (bytesToRead > 0 && streamBufferSize > 0) {

				auto newPos = streamBufferPosition + bytesToRead;
				if (newPos <= streamBufferSize) {

					out = std::copy_n(streamBufferData + streamBufferPosition, bytesToRead, out);
					streamBufferPosition = newPos;
					bytesToRead = 0;
					break;
				}

				out = std::copy_n(streamBufferData + streamBufferPosition, streamBufferSize - streamBufferPosition, out);
				streamBufferPosition = streamBufferSize;
				bytesToRead = newPos - streamBufferSize;

				streamBuffer = co_await baseStream.ReadAsync(streamBuffer, StreamBufferCapacity, {});
				streamBufferPosition = 0;
				streamBufferSize = streamBuffer.Length();
				streamBufferData = streamBuffer.data();
			}

			buffer.Length(count - bytesToRead);
		}

		auto Skip(std::uint64_t count) -> void {

			bytesToSkip += count;
		}

	private:

		winrt::IInputStream baseStream{ nullptr };
		winrt::IBuffer streamBuffer{ nullptr };
		std::uint64_t streamBufferPosition{};
		std::uint64_t bytesToSkip{};
	};

	class BufferReader {
	public:

		BufferReader(std::span<std::uint8_t> buffer) noexcept

			: bufferData(buffer.data())
			, bufferSize(static_cast<std::uint32_t>(buffer.size()))
		{}

		BufferReader(winrt::IBuffer& buffer)

			: bufferData(buffer.data())
			, bufferSize(buffer.Length())
		{}

		BufferReader(BufferReader const&) = delete;
		auto operator=(BufferReader const&) = delete;

		auto BufferData() noexcept -> std::uint8_t* {

			return bufferData;
		}

		auto BufferSize() noexcept -> std::uint32_t {

			return bufferSize;
		}

		template<typename T>
		auto Read() noexcept -> T* {

			static_assert(std::has_unique_object_representations_v<T>);
			static_assert(std::is_trivially_copy_assignable_v<T>);
			static_assert(alignof(T) == 1);
			static_assert(std::endian::native == std::endian::little);

			auto byteSize = sizeof(T);
			if (static_cast<std::uint64_t>(position) + byteSize > bufferSize)
				return nullptr;

			auto obj = StartLifetimeAs<T>(bufferData + position);
			position += byteSize;
			return obj;
		}

		template<typename T>
		auto ReadArray(std::uint32_t n) noexcept -> std::span<T> {

			static_assert(std::has_unique_object_representations_v<T>);
			static_assert(std::is_trivially_copy_assignable_v<T>);
			static_assert(alignof(T) == 1);
			static_assert(std::endian::native == std::endian::little);

			auto byteSize = sizeof(T) * n;
			if (static_cast<std::uint64_t>(position) + byteSize > bufferSize)
				return {};

			auto objArr = StartLifetimeAsArray<T>(bufferData + position, n);
			position += byteSize;
			return { objArr, n };
		}

		template<typename T>
		auto ReadInto(T& obj) noexcept -> bool {

			static_assert(std::has_unique_object_representations_v<T>);
			static_assert(std::is_trivially_copy_assignable_v<T>);
			static_assert(std::endian::native == std::endian::little);

			auto byteSize = sizeof(T);
			if (static_cast<std::uint64_t>(position) + byteSize > bufferSize)
				return false;

			std::copy_n(bufferData + position, sizeof(T), reinterpret_cast<std::uint8_t*>(&obj));
			position += byteSize;
			return true;
		}

		auto Skip(std::uint32_t count) noexcept -> void {

			position += count;
		}

		auto Position() noexcept -> std::uint32_t {

			return position;
		}

		auto Seek(std::uint32_t newPos) noexcept -> void {

			position = newPos;
		}

	private:

		std::uint8_t* bufferData{};
		std::uint32_t bufferSize{};
		std::uint32_t position{};
	};

	constexpr auto DefaultBlockSize = 0x10000;

	constexpr auto& ManifestFileName = "AppxManifest.xml";
	constexpr auto& BlockMapFileName = "AppxBlockMap.xml";

	struct alignas(8) ExtractionContext {

		std::uint64_t FileNumber{};
	};

	class ExtractionContextStore {
	public:

		static constexpr auto FileCookie = "843356e9-94e6-4cee-b39d-19614adcbb04"_Guid;
		static constexpr auto SerializedSize = sizeof(FileCookie) + sizeof(ExtractionContext) + 0x20;

		static auto Open(File&& contextFile, Windows::PackageIdentity const& packageId, std::filesystem::path const& destinationPath) -> std::shared_ptr<ExtractionContextStore> {

			class ConcreteImpl : public ExtractionContextStore {
			public:

				ConcreteImpl(File&& contextFile, Windows::PackageIdentity const& packageId, std::filesystem::path const& destinationPath)

					: ExtractionContextStore(std::move(contextFile), packageId, destinationPath)
				{}
			};

			auto contextStore = std::make_shared<ConcreteImpl>(std::move(contextFile), packageId, destinationPath);
			contextStore->Initialize();
			return contextStore;
		}

		auto Load() const noexcept -> ExtractionContext {

			return std::atomic_ref{ context }.load(std::memory_order::acquire);
		}

		auto Store(ExtractionContext contextValue) noexcept -> void {

			std::atomic_ref{ context }.store(contextValue, std::memory_order::release);
		}

		auto Close() {

			auto lock = std::scoped_lock{ mutex };
			if (!file)
				return;

			Serialize();
			file.Close();
		}

		~ExtractionContextStore() {

			Close();
		}

	protected:

		ExtractionContextStore(File&& contextFile, Windows::PackageIdentity const& packageId, std::filesystem::path const& destinationPath)

			: file(std::move(contextFile))
			, packageFullName(packageId.FullName())
			, destinationPath(destinationPath.string())
			, hashFunction(Botan::HashFunction::create("SHA-256"))
		{}

		ExtractionContextStore(ExtractionContextStore const&) = delete;
		auto operator=(ExtractionContextStore const&) = delete;

		auto Initialize() -> void {

			Parse();
		}

		auto Parse() noexcept -> void {

			auto alignas(8) buffer = TrivialArray<std::uint8_t, SerializedSize>{};
			auto bytesRead = 0uz;

			if (!file.Read(buffer, bytesRead) || bytesRead != SerializedSize)
				return;

			if (*StartLifetimeAs<Guid>(&buffer[0]) != FileCookie)
				return;

			auto contextValue = *StartLifetimeAs<ExtractionContext>(&buffer[0x10]);
			auto checksum = std::span{ buffer.begin() + 0x18, buffer.end() };

			auto hash = TrivialArray<std::uint8_t, 0x20>{};
			CalculateHash(contextValue, hash.data());

			if (!std::ranges::equal(hash, checksum))
				return;

			context = contextValue;
		}

		auto Serialize() noexcept -> void {

			auto buffer = TrivialArray<std::uint8_t, SerializedSize>{};
			auto out = buffer.data();

			auto contextValue = std::atomic_ref{ context }.load(std::memory_order::acquire);
			auto hash = TrivialArray<std::uint8_t, 0x20>{};
			CalculateHash(contextValue, hash.data());

			out = std::copy_n(reinterpret_cast<std::uint8_t const*>(&FileCookie), sizeof(FileCookie), out);
			out = std::copy_n(reinterpret_cast<std::uint8_t const*>(&contextValue), sizeof(contextValue), out);
			out = std::copy_n(hash.data(), hash.size(), out);

			file.SeekToBegin();
			file.Write(buffer);
		}

		auto CalculateHash(ExtractionContext contextValue, std::uint8_t* out) const noexcept -> void {

			hashFunction->update(reinterpret_cast<std::uint8_t const*>(&FileCookie), sizeof(FileCookie));
			hashFunction->update(reinterpret_cast<std::uint8_t const*>(&contextValue), sizeof(contextValue));
			hashFunction->update(reinterpret_cast<std::uint8_t const*>(packageFullName.data()), packageFullName.size());
			hashFunction->update(reinterpret_cast<std::uint8_t const*>(destinationPath.data()), destinationPath.size());
			hashFunction->final(out);
		}

		ExtractionContext context;

		File file;
		std::string packageFullName;
		std::string destinationPath;

		std::unique_ptr<Botan::HashFunction> hashFunction{ nullptr };
		std::shared_mutex mutex;
	};
}

namespace Citrine::Windows {

	class StreamedMsixFile::Impl : public std::enable_shared_from_this<Impl> {
	public:

		Impl(Impl const&) = delete;
		auto operator=(Impl const&) = delete;

		static auto OpenFromFileAsync(File&& file) -> AsyncMsixOperationResult<StreamedMsixFile> {

			return OpenFromStreamAsync(MakeWinRTFileStream(std::move(file)));
		}

		static auto OpenFromStreamAsync(winrt::IRandomAccessStream&& stream) -> AsyncMsixOperationResult<StreamedMsixFile> {

			class ConcreteImpl : public Impl {
			public:

				ConcreteImpl(winrt::IRandomAccessStream&& stream) : Impl(std::move(stream)) {}
			};

			auto impl = std::static_pointer_cast<Impl>(std::make_shared<ConcreteImpl>(std::move(stream)));
			auto result = co_await impl->InitializeAsync();

			if (!result)
				co_return result.error();
			co_return StreamedMsixFile{ std::move(impl) };
		}

		auto PackageManifest() const noexcept -> MsixManifest const& {

			return manifest;
		}

		auto ExtractAllFilesAsync(std::filesystem::path destinationDirectory, MsixExtractionProgressCallback progressCallback, File contextFile) -> AsyncMsixOperationResult<> try {

			if (!destinationDirectory.is_absolute())
				throw std::invalid_argument{ "Absolute path required" };

			if (!stream)
				co_return MsixError::StreamNotOpen;

			auto strongSelf = shared_from_this();
			co_await winrt::resume_background();

			auto contextStore = ExtractionContextStore::Open(std::move(contextFile), manifest.Identity(), destinationDirectory);
			auto context = contextStore->Load();

			auto cancellationToken = co_await GetCancellationToken();
			cancellationToken.Callback([contextStore] { contextStore->Close(); });

			auto getAbsolutePath = [&rootDirectory = destinationDirectory](std::string_view path) {

				auto absolutePath = std::wstring{};
				absolutePath.reserve(rootDirectory.native().size() + 1 + path.size());
				absolutePath.append(rootDirectory.native());

				if (absolutePath.back() != L'\\' && absolutePath.back() != '/')
					absolutePath.push_back(L'\\');

				ToUtf16(path, AppendTo(absolutePath));
				return std::filesystem::path{ std::move(absolutePath) };
			};

			auto createParentDirectories = [&, directories = std::unordered_set<std::string_view>{}](this auto& self, std::string_view path) -> bool {

				if (auto pos = path.find_last_of("\\/"); pos != path.npos) {

					auto parentDirectory = std::string_view{ path.data(), pos };
					if (directories.contains(parentDirectory))
						return true;

					if (!self(parentDirectory))
						return false;

					auto ec = std::error_code{};
					if (std::filesystem::create_directory(getAbsolutePath(parentDirectory), ec); ec)
						return false;

					directories.emplace(parentDirectory);
				}

				return true;
			};

			auto openFile = [&](std::string_view path) -> File {

				return File{ getAbsolutePath(path), FileMode::OpenAlways, FileAccess::Write };
			};

			auto progress = MsixExtractionProgress{};
			progress.TotalBytesToProcess = uncompressedPayloadSize;

			auto fileEntries = blockMap.child("BlockMap");
			auto currentFileEntry = fileEntries.begin();

			auto cdReader = BufferReader{ cdBuffer };
			auto payloadStreamOffset = std::uint64_t{};

			for (auto fileNumber = std::uint64_t{}; fileNumber < context.FileNumber; ++fileNumber) {

				if (currentFileEntry == fileEntries.end())
					break;

				auto fileSize = currentFileEntry->attribute("Size").as_ullong();
				payloadStreamOffset += currentFileEntry->attribute("LfhSize").as_uint();

				auto cdfh = cdReader.Read<ZipCentralDirectoryFileHeader>();
				if (!cdfh)
					co_return MsixError::ParsingFailed;

				cdReader.Skip(cdfh->FileNameLength);
				auto extraField = cdReader.ReadArray<std::uint8_t>(cdfh->ExtraFieldLength);
				cdReader.Skip(cdfh->FileCommentLength);

				auto extendedInfo = Zip64ExtendedInfo{};
				if (!ReadZip64ExtendedInfo(cdfh, extraField, extendedInfo))
					co_return MsixError::ParsingFailed;

				payloadStreamOffset += extendedInfo.CompressedSize;

				if ((cdfh->Flags & ZipFlags::DataDescriptor) == ZipFlags::DataDescriptor) {

					payloadStreamOffset += sizeof(Zip64DataDescriptor);
				}

				progress.BytesProcessed += fileSize;
				++currentFileEntry;
			}

			auto blockBuffer = winrt::IBuffer{ winrt::Buffer{ DefaultBlockSize + 0x100 } }; // Accommodate for deflate overhead
			auto uncompressedBlockBuffer = winrt::IBuffer{ winrt::Buffer{ DefaultBlockSize } };

			auto payloadStream = [&] -> BufferedInputStream {

				if (auto rangeStreamProvider = stream.try_as<winrt::Citrine::IRangeStreamProvider>()) {

					return rangeStreamProvider.GetRangeStream(payloadStreamOffset, blockMapFileInfo->LocalHeaderOffset);
				}
				else {

					return stream.GetInputStreamAt(payloadStreamOffset);
				}
			}();

			auto inflateStream = zng_stream{};
			if (::zng_inflateInit2(&inflateStream, -MAX_WBITS) != Z_OK)
				co_return MsixError::DecompressionFailed;

			auto closeInflateStream = ScopeExit{ [&] { ::zng_inflateEnd(&inflateStream); } };

			while (currentFileEntry != fileEntries.end()) {

				contextStore->Store(context);

				auto cdfh = cdReader.Read<ZipCentralDirectoryFileHeader>();
				if (!cdfh)
					co_return MsixError::ParsingFailed;

				auto fileSize = currentFileEntry->attribute("Size").as_ullong();
				auto filePath = std::string_view{ currentFileEntry->attribute("Name").as_string() };
				payloadStream.Skip(currentFileEntry->attribute("LfhSize").as_uint());

				if (!createParentDirectories(filePath))
					co_return MsixError::WritingFailed;

				auto file = openFile(filePath);
				if (!file)
					co_return MsixError::WritingFailed;

				auto blocks = currentFileEntry->children();
				auto currentBlock = blocks.begin();

				if (::zng_inflateReset(&inflateStream) != Z_OK)
					co_return MsixError::DecompressionFailed;

				auto currentFilePosition = std::uint64_t{};
				while (currentFilePosition < fileSize) {

					if (currentBlock == blocks.end())
						co_return MsixError::DataIntegrityViolation;

					auto blockSize = std::uint32_t{};
					if (cdfh->CompressionMethod == ZipCompressionMethod::Deflate) {

						auto blockSizeAttribute = currentBlock->attribute("Size");
						if (!blockSizeAttribute)
							co_return MsixError::ParsingFailed;

						blockSize = blockSizeAttribute.as_uint();
					}
					else {

						blockSize = std::min(fileSize - currentFilePosition, std::uint64_t{ DefaultBlockSize });
					}

					co_await payloadStream.ReadAsync(blockBuffer, blockSize);
					auto blockSpan = std::span{ blockBuffer.data(), blockBuffer.Length() };

					if (blockSpan.size() != blockSize)
						co_return MsixError::ParsingFailed;

					auto uncompressedBlockSpan = std::span{ uncompressedBlockBuffer.data(), std::min(fileSize - currentFilePosition, std::uint64_t{ DefaultBlockSize }) };

					if (cdfh->CompressionMethod == ZipCompressionMethod::Deflate) {

						inflateStream.next_in = blockSpan.data();
						inflateStream.avail_in = static_cast<std::uint32_t>(blockSpan.size());
						inflateStream.next_out = uncompressedBlockSpan.data();
						inflateStream.avail_out = static_cast<std::uint32_t>(uncompressedBlockSpan.size());

						auto result = ::zng_inflate(&inflateStream, Z_SYNC_FLUSH);
						if (result != S_OK && result != Z_STREAM_END)
							co_return MsixError::DecompressionFailed;
					}
					else {

						uncompressedBlockSpan = blockSpan;
					}

					auto blockHashAttribute = currentBlock->attribute("Hash");
					if (!blockHashAttribute)
						co_return MsixError::ParsingFailed;

					auto blockHash = std::string_view{ blockHashAttribute.as_string() };
					auto checksum = TrivialArray<std::uint8_t, 0x40>{};
					if (Base64::DecodedSize(blockHash) != hashSize || !Base64::Decode(blockHash, checksum.data()))
						co_return MsixError::DataIntegrityViolation;

					auto digest = TrivialArray<std::uint8_t, 0x40>{};
					hashFunction->update(uncompressedBlockSpan);
					hashFunction->final(digest.data());

					if (!std::ranges::equal(std::span{ digest.data(), hashSize }, std::span{ checksum.data(), hashSize }))
						co_return MsixError::DataIntegrityViolation;

					if (!file.Write(uncompressedBlockSpan))
						co_return MsixError::WritingFailed;

					currentFilePosition += uncompressedBlockSpan.size();
					progress.BytesProcessed += uncompressedBlockSpan.size();

					if (progressCallback)
						progressCallback(progress);

					++currentBlock;
				};

				if (!file.Truncate())
					co_return MsixError::WritingFailed;

				cdReader.Skip(cdfh->FileNameLength);
				auto extraField = cdReader.ReadArray<std::uint8_t>(cdfh->ExtraFieldLength);
				cdReader.Skip(cdfh->FileCommentLength);

				if (cdfh->CompressionMethod == ZipCompressionMethod::Deflate) {

					auto extendedInfo = Zip64ExtendedInfo{};
					if (!ReadZip64ExtendedInfo(cdfh, extraField, extendedInfo))
						co_return MsixError::ParsingFailed;

					if (extendedInfo.CompressedSize < inflateStream.total_in)
						co_return MsixError::ParsingFailed;

					payloadStream.Skip(extendedInfo.CompressedSize - inflateStream.total_in);
				}

				if ((cdfh->Flags & ZipFlags::DataDescriptor) == ZipFlags::DataDescriptor) {

					payloadStream.Skip(sizeof(Zip64DataDescriptor));
				}

				++currentFileEntry;
				++context.FileNumber;
			}

			co_return {};
		}
		catch (winrt::hresult_error const&) {

			co_return MsixError::ReadingFailed;
		}

		auto DetachStream() noexcept -> winrt::IRandomAccessStream {

			return std::move(stream);
		}

	private:

		Impl(winrt::IRandomAccessStream&& stream)

			: stream(std::move(stream))
		{}

		auto InitializeAsync() -> AsyncMsixOperationResult<> {

			return ParseAsync();
		}

		auto ParseAsync() -> AsyncMsixOperationResult<> {

			if (!stream)
				co_return MsixError::StreamNotOpen;

			auto strongSelf = shared_from_this();
			co_await winrt::resume_background();

			if (auto result = co_await ParseCentralDirectoryAsync(); !result)
				co_return result.error();

			if (!blockMapFileInfo || !manifestFileInfo)
				co_return MsixError::UnsupportedFormat;

			if (auto result = co_await ParseBlockMapAsync(); !result)
				co_return result.error();

			if (auto result = co_await ParseManifestAsync(); !result)
				co_return result.error();

			co_return {};
		}

		auto ParseCentralDirectoryAsync() -> AsyncMsixOperationResult<> try {

			constexpr auto eocdDataSize = sizeof(Zip64EndOfCentralDirectory) + sizeof(Zip64EndOfCentralDirectoryLocator) + sizeof(ZipEndOfCentralDirectory);

			auto streamSize = stream.Size();
			if (eocdDataSize > streamSize)
				co_return MsixError::ParsingFailed;

			auto eocdDataOffset = streamSize - eocdDataSize;

			stream.Seek(eocdDataOffset);
			auto eocdDataBuffer = co_await stream.ReadAsync(winrt::Buffer{ eocdDataSize }, eocdDataSize, {});
			auto eocdDataReader = BufferReader{ eocdDataBuffer };

			eocdDataReader.Seek(sizeof(Zip64EndOfCentralDirectory) + sizeof(Zip64EndOfCentralDirectoryLocator));
			auto eocd = eocdDataReader.Read<ZipEndOfCentralDirectory>();
			if (!eocd || eocd->Signature != ZipEndOfCentralDirectory::ExpectedSignature)
				co_return MsixError::ParsingFailed;

			eocdDataReader.Seek(sizeof(Zip64EndOfCentralDirectory));
			auto eocd64Locator = eocdDataReader.Read<Zip64EndOfCentralDirectoryLocator>();
			if (!eocd64Locator || eocd64Locator->Signature != Zip64EndOfCentralDirectoryLocator::ExpectedSignature)
				co_return MsixError::ParsingFailed;

			if (eocd64Locator->Offset != eocdDataOffset)
				co_return MsixError::UnsupportedFormat;

			eocdDataReader.Seek(0);
			auto eocd64 = eocdDataReader.Read<Zip64EndOfCentralDirectory>();
			if (!eocd64 || eocd64->Signature != Zip64EndOfCentralDirectory::ExpectedSignature)
				co_return MsixError::ParsingFailed;

			auto cdOffset = eocd64->CentralDirectoryOffset;
			auto cdSize = eocd64->CentralDirectorySize;

			if (cdOffset + cdSize > eocdDataOffset)
				co_return MsixError::ParsingFailed;

			if (cdSize > 0xFFFFFFFF)
				co_return MsixError::UnsupportedFormat;

			stream.Seek(cdOffset);
			cdBuffer = co_await stream.ReadAsync(winrt::Buffer{ static_cast<std::uint32_t>(cdSize) }, static_cast<std::uint32_t>(cdSize), {});
			auto cdReader = BufferReader{ cdBuffer };

			while (cdReader.Position() < cdReader.BufferSize()) {

				auto cdfh = cdReader.Read<ZipCentralDirectoryFileHeader>();
				if (!cdfh || cdfh->Signature != ZipCentralDirectoryFileHeader::ExpectedSignature)
					co_return MsixError::ParsingFailed;

				auto fileName = std::string_view{ cdReader.ReadArray<char>(cdfh->FileNameLength) };
				if (!fileName.data())
					co_return MsixError::ParsingFailed;

				auto extraField = cdReader.ReadArray<std::uint8_t>(cdfh->ExtraFieldLength);
				if (!extraField.data())
					co_return MsixError::ParsingFailed;

				if (fileName == BlockMapFileName) {

					auto extendedInfo = Zip64ExtendedInfo{};
					if (!ReadZip64ExtendedInfo(cdfh, extraField, extendedInfo))
						co_return MsixError::ParsingFailed;

					blockMapFileInfo.emplace(
						cdfh->Flags,
						cdfh->CompressionMethod,
						cdfh->Crc32,
						extendedInfo.UncompressedSize,
						extendedInfo.CompressedSize,
						extendedInfo.LocalHeaderOffset
					);

					if (!manifestFileInfo)
						co_return MsixError::ParsingFailed;

					break;
				}

				if (fileName == ManifestFileName) {

					if (manifestFileInfo)
						co_return MsixError::ParsingFailed;

					auto extendedInfo = Zip64ExtendedInfo{};
					if (!ReadZip64ExtendedInfo(cdfh, extraField, extendedInfo))
						co_return MsixError::ParsingFailed;

					manifestFileInfo.emplace(
						cdfh->Flags,
						cdfh->CompressionMethod,
						cdfh->Crc32,
						extendedInfo.UncompressedSize,
						extendedInfo.CompressedSize,
						extendedInfo.LocalHeaderOffset
					);
				}

				cdReader.Skip(cdfh->FileCommentLength);
			}

			co_return {};
		}
		catch (winrt::hresult_error const&) {

			co_return MsixError::ReadingFailed;
		}

		auto ParseBlockMapAsync() -> AsyncMsixOperationResult<> {

			auto fileContent = co_await ExtractFootprintFileAsync(&*blockMapFileInfo);
			if (!fileContent)
				co_return fileContent.error();

			blockMapBuffer = std::move(*fileContent);
			if (!blockMap.load_buffer_inplace(blockMapBuffer.data(), blockMapBuffer.Length()))
				co_return MsixError::ParsingFailed;

			auto blockMapElement = blockMap.child("BlockMap");
			if (!blockMapElement)
				co_return MsixError::ParsingFailed;

			auto hashMethodAttribute = blockMapElement.attribute("HashMethod");
			if (!hashMethodAttribute)
				co_return MsixError::ParsingFailed;

			auto hashMethodName = UrlView{ hashMethodAttribute.as_string() }.Fragment();
			if (hashMethodName == "sha256") {

				hashFunction = Botan::HashFunction::create("SHA-256");
				hashSize = 0x20;
			}
			else if (hashMethodName == "sha384") {

				hashFunction = Botan::HashFunction::create("SHA-384");
				hashSize = 0x30;
			}
			else if (hashMethodName == "sha512") {

				hashFunction = Botan::HashFunction::create("SHA-512");
				hashSize = 0x40;
			}
			else {

				co_return MsixError::UnsupportedFormat;
			}

			for (auto const& fileEntry : blockMapElement) {

				auto nameAttribute = fileEntry.attribute("Name");
				auto sizeAttribute = fileEntry.attribute("Size");
				auto lfhSizeAttribute = fileEntry.attribute("LfhSize");

				if (!nameAttribute || !sizeAttribute || !lfhSizeAttribute)
					co_return MsixError::ParsingFailed;

				if (std::string_view{ nameAttribute.as_string() } == ManifestFileName) {

					if (manifestFileInfo->BlockMapEntry)
						co_return MsixError::ParsingFailed;

					manifestFileInfo->BlockMapEntry = fileEntry;
				}

				uncompressedPayloadSize += sizeAttribute.as_ullong();
			}

			if (!manifestFileInfo->BlockMapEntry)
				co_return MsixError::ParsingFailed;

			co_return {};
		}

		auto ParseManifestAsync() -> AsyncMsixOperationResult<> {

			auto fileContent = co_await ExtractFootprintFileAsync(&*manifestFileInfo);
			if (!fileContent)
				co_return fileContent.error();

			auto result = MsixManifest::OpenFromBuffer(std::move(*fileContent));
			if (!result)
				co_return MsixError::InvalidPackageManifest;

			manifest = std::move(*result);
			co_return {};
		}

		struct Zip64ExtendedInfo {

			std::uint64_t UncompressedSize{};
			std::uint64_t CompressedSize{};
			std::uint64_t LocalHeaderOffset{};
		};

		auto ReadZip64ExtendedInfo(ZipCentralDirectoryFileHeader* cdfh, std::span<std::uint8_t> extraField, Zip64ExtendedInfo& extendedInfo) noexcept -> bool {

			auto extraFieldReader = BufferReader{ extraField };
			auto extendedInfoSpan = std::span<std::uint8_t>{};

			while (extraFieldReader.Position() < extraFieldReader.BufferSize()) {

				auto efh = extraFieldReader.Read<ZipExtraFieldHeader>();
				if (!efh)
					return false;

				if (efh->Id != 0x0001) {

					extraFieldReader.Skip(efh->Size);
					continue;
				}

				extendedInfoSpan = extraFieldReader.ReadArray<std::uint8_t>(efh->Size);
				if (!extendedInfoSpan.data())
					return false;

				break;
			}

			auto extendedInfoReader = BufferReader{ extendedInfoSpan };

			extendedInfo.UncompressedSize = cdfh->UncompressedSize;
			if (extendedInfo.UncompressedSize == 0xFFFFFFFF && !extendedInfoReader.ReadInto(extendedInfo.UncompressedSize))
				return false;

			extendedInfo.CompressedSize = cdfh->CompressedSize;
			if (extendedInfo.CompressedSize == 0xFFFFFFFF && !extendedInfoReader.ReadInto(extendedInfo.CompressedSize))
				return false;

			extendedInfo.LocalHeaderOffset = cdfh->LocalHeaderOffset;
			if (extendedInfo.LocalHeaderOffset == 0xFFFFFFFF && !extendedInfoReader.ReadInto(extendedInfo.LocalHeaderOffset))
				return false;

			return true;
		}

		struct FileInfo {

			ZipFlags Flags;
			ZipCompressionMethod CompressionMethod;
			std::uint32_t Crc32{};
			std::uint64_t UncompressedSize{};
			std::uint64_t CompressedSize{};
			std::uint64_t LocalHeaderOffset{};
			pugi::xml_node BlockMapEntry;
		};

		auto ExtractFootprintFileAsync(FileInfo const* fileInfo) -> AsyncMsixOperationResult<winrt::IBuffer> try {

			if (fileInfo->UncompressedSize > 0xFFFFFFFF || fileInfo->CompressedSize > 0xFFFFFFFF)
				co_return MsixError::UnsupportedFormat;

			auto lfhOffset = fileInfo->LocalHeaderOffset;
			auto lfhSize = std::uint32_t{};
			auto fileSize = static_cast<std::uint32_t>(fileInfo->UncompressedSize);

			if (fileInfo->BlockMapEntry) {

				lfhSize = fileInfo->BlockMapEntry.attribute("LfhSize").as_uint();
			}
			else {

				stream.Seek(fileInfo->LocalHeaderOffset);
				auto lfhBuffer = co_await stream.ReadAsync(winrt::Buffer{ sizeof(ZipLocalFileHeader) }, sizeof(ZipLocalFileHeader), {});
				auto lfhReader = BufferReader{ lfhBuffer };

				auto lfh = lfhReader.Read<ZipLocalFileHeader>();
				if (!lfh || lfh->Signature != ZipLocalFileHeader::ExpectedSignature)
					co_return MsixError::ParsingFailed;

				lfhSize = sizeof(ZipLocalFileHeader) + lfh->FileNameLength + lfh->ExtraFieldLength;
			}

			constexpr auto bufferCapacity = std::uint32_t{ 0x10000 };
			auto buffer = winrt::IBuffer{ winrt::Buffer{ bufferCapacity } };

			auto fileContentBuffer = winrt::Buffer{ static_cast<std::uint32_t>(fileSize) };
			auto fileContentSpan = std::span{ fileContentBuffer.data(), fileSize };

			auto fileStream = winrt::IInputStream{ nullptr };
			if (auto rangeStreamProvider = stream.try_as<winrt::Citrine::IRangeStreamProvider>()) {

				fileStream = rangeStreamProvider.GetRangeStream(lfhOffset + lfhSize, fileInfo->CompressedSize);
			}
			else {

				fileStream = stream.GetInputStreamAt(lfhOffset + lfhSize);
			}

			auto inflateStream = zng_stream{};
			if (::zng_inflateInit2(&inflateStream, -MAX_WBITS) != Z_OK)
				co_return MsixError::DecompressionFailed;

			auto closeInflateStream = ScopeExit{ [&] { ::zng_inflateEnd(&inflateStream); } };

			inflateStream.next_out = fileContentSpan.data();
			inflateStream.avail_out = static_cast<std::uint32_t>(fileContentSpan.size());

			auto currentFilePosition = std::uint32_t{};
			while (currentFilePosition < fileSize) {

				buffer = co_await fileStream.ReadAsync(buffer, std::min(fileSize - currentFilePosition, bufferCapacity), {});
				auto bufferSpan = std::span{ buffer.data(), buffer.Length() };

				if (bufferSpan.empty())
					co_return MsixError::ReadingFailed;

				if (fileInfo->CompressionMethod == ZipCompressionMethod::Deflate) {

					inflateStream.next_in = bufferSpan.data();
					inflateStream.avail_in = static_cast<std::uint32_t>(bufferSpan.size());

					auto result = ::zng_inflate(&inflateStream, Z_SYNC_FLUSH);
					if (result != S_OK && result != Z_STREAM_END)
						co_return MsixError::DecompressionFailed;

					currentFilePosition = inflateStream.total_out;
				}
				else {

					std::ranges::copy(bufferSpan, fileContentSpan.data() + currentFilePosition);
					currentFilePosition += bufferSpan.size();
				}
			};

			if (fileInfo->BlockMapEntry) {

				auto currentBlock = fileInfo->BlockMapEntry.begin();
				auto bytesProcessed = 0uz;

				while (bytesProcessed < fileContentSpan.size()) {

					if (currentBlock == fileInfo->BlockMapEntry.end())
						co_return MsixError::DataIntegrityViolation;

					auto uncompressedBlockSpan = std::span{ fileContentSpan.data() + bytesProcessed, std::min(fileContentSpan.size() - bytesProcessed, std::size_t{ DefaultBlockSize }) };

					auto blockHashAttribute = currentBlock->attribute("Hash");
					if (!blockHashAttribute)
						co_return MsixError::ParsingFailed;

					auto blockHash = std::string_view{ blockHashAttribute.as_string() };
					auto checksum = TrivialArray<std::uint8_t, 0x40>{};
					if (Base64::DecodedSize(blockHash) != hashSize || !Base64::Decode(blockHash, checksum.data()))
						co_return MsixError::DataIntegrityViolation;

					auto digest = TrivialArray<std::uint8_t, 0x40>{};
					hashFunction->update(uncompressedBlockSpan);
					hashFunction->final(digest.data());

					if (!std::ranges::equal(std::span{ digest.data(), hashSize }, std::span{ checksum.data(), hashSize }))
						co_return MsixError::DataIntegrityViolation;

					++currentBlock;
					bytesProcessed += uncompressedBlockSpan.size();
				}
			}
			else {

				if (::zng_crc32_z(0, fileContentSpan.data(), fileContentSpan.size()) != fileInfo->Crc32)
					co_return MsixError::DataIntegrityViolation;
			}

			fileContentBuffer.Length(fileInfo->UncompressedSize);
			co_return fileContentBuffer;
		}
		catch (winrt::hresult_error const&) {

			co_return MsixError::ReadingFailed;
		}

		winrt::IRandomAccessStream stream{ nullptr };
		std::uint64_t uncompressedPayloadSize{};

		winrt::IBuffer cdBuffer{ nullptr };

		winrt::IBuffer blockMapBuffer{ nullptr };
		pugi::xml_document blockMap;
		std::unique_ptr<Botan::HashFunction> hashFunction{ nullptr };
		std::size_t hashSize{};

		std::optional<FileInfo> blockMapFileInfo;
		std::optional<FileInfo> manifestFileInfo;
		MsixManifest manifest{ nullptr };
	};

	StreamedMsixFile::StreamedMsixFile(std::shared_ptr<Impl>&& impl) noexcept

		: impl(std::move(impl))
	{}

	auto StreamedMsixFile::operator=(std::nullptr_t) noexcept -> StreamedMsixFile& {

		impl.reset();
		return *this;
	}

	auto StreamedMsixFile::OpenFromFileAsync(File&& file) -> AsyncMsixOperationResult<StreamedMsixFile> {

		return Impl::OpenFromFileAsync(std::move(file));
	}

	auto StreamedMsixFile::OpenFromStreamAsync(winrt::Windows::Storage::Streams::IRandomAccessStream&& stream) -> AsyncMsixOperationResult<StreamedMsixFile> {

		return Impl::OpenFromStreamAsync(std::move(stream));
	}

	auto StreamedMsixFile::PackageManifest() const noexcept -> MsixManifest const& {

		return impl->PackageManifest();
	}

	auto StreamedMsixFile::ExtractAllFilesAsync(std::filesystem::path destinationDirectory, MsixExtractionProgressCallback progressCallback, File contextFile) -> AsyncMsixOperationResult<> {

		return impl->ExtractAllFilesAsync(std::move(destinationDirectory), std::move(progressCallback), std::move(contextFile));
	}

	StreamedMsixFile::operator bool() const noexcept {

		return static_cast<bool>(impl);
	}

	auto StreamedMsixFile::Release() noexcept -> void {

		impl.reset();
	}

	auto StreamedMsixFile::Stream() && noexcept -> winrt::Windows::Storage::Streams::IRandomAccessStream {

		return impl->DetachStream();
	}

	auto StreamedMsixFile::swap(StreamedMsixFile& other) noexcept -> void {

		impl.swap(other.impl);
	}
}