#include "pch.h"
#include "StreamedXvcFile.h"

#include "XvdHeader.h"
#include "XvdUserDataHeader.h"
#include "XvdUserDataPackageFilesHeader.h"
#include "XvdUserDataPackageFile.h"
#include "XvdSegmentMetadata.h"
#include "XvcHeader.h"
#include "XvcRegionHeader.h"
#include "XvcSegment.h"
#include "XvcRegionSpecifier.h"

#include "Core/IO/WinRTFileStream.h"
#include "Core/Util/Memory.h"
#include "Core/Util/TrivialArray.h"

#include <span>
#include <type_traits>
#include <algorithm>
#include <bit>
#include <map>
#include <unordered_set>
#include <atomic>
#include <shared_mutex>

#include <winrt/Citrine.h>

#include <botan/cipher_mode.h>
#include <botan/hash.h>

namespace winrt {

	using namespace Windows::Storage::Streams;
}

using namespace Citrine;
using namespace Xbox;

namespace {

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
			static_assert(std::endian::native == std::endian::little);

			auto byteSize = sizeof(T);
			if (static_cast<std::uint64_t>(position) + byteSize > bufferSize)
				return nullptr;

			if constexpr (alignof(T) != 1) {

				if ((std::bit_cast<std::uintptr_t>(bufferData + position) & (alignof(T) - 1)) != 0)
					return nullptr;
			}

			auto obj = StartLifetimeAs<T>(bufferData + position);
			position += byteSize;
			return obj;
		}

		template<typename T>
		auto ReadArray(std::uint32_t n) noexcept -> std::span<T> {

			static_assert(std::has_unique_object_representations_v<T>);
			static_assert(std::is_trivially_copy_assignable_v<T>);
			static_assert(std::endian::native == std::endian::little);

			auto byteSize = sizeof(T) * n;
			if (static_cast<std::uint64_t>(position) + byteSize > bufferSize)
				return {};

			if constexpr (alignof(T) != 1) {

				if ((std::bit_cast<std::uintptr_t>(bufferData + position) & (alignof(T) - 1)) != 0)
					return {};
			}

			auto objArr = StartLifetimeAsArray<T>(bufferData + position, n);
			position += byteSize;
			return { objArr, n };
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

	constexpr auto PageSize = 0x1000;
	constexpr auto UnusedSpace = 0x2000;
	constexpr auto HashEntryLength = 0x18;
	constexpr auto HashEntriesPerHashBlock = PageSize / HashEntryLength;

	constexpr auto DataBlocksPerLevel0HashTree = HashEntriesPerHashBlock;
	constexpr auto DataBlocksPerLevel1HashTree = HashEntriesPerHashBlock * DataBlocksPerLevel0HashTree;
	constexpr auto DataBlocksPerLevel2HashTree = HashEntriesPerHashBlock * DataBlocksPerLevel1HashTree;
	constexpr auto DataBlocksPerLevel3HashTree = HashEntriesPerHashBlock * DataBlocksPerLevel2HashTree;

	auto BytesToPages(std::uint64_t bytes) noexcept -> std::uint64_t {

		return bytes / PageSize;
	}

	auto AlignBytesToPages(std::uint64_t bytes) noexcept -> std::uint64_t {

		return (bytes + PageSize - 1) & ~(static_cast<std::uint64_t>(PageSize) - 1);
	}

	auto PagesToBytes(std::uint64_t pages) noexcept -> std::uint64_t {

		return pages * PageSize;
	}

	auto NumberOfHashPagesForLevel(std::uint64_t dataPages, int level) noexcept -> std::uint64_t {

		if (dataPages == 0)
			return 0;

		if (level > 0) {

			if (NumberOfHashPagesForLevel(dataPages, level - 1) <= 1)
				return 0;
		}

		auto divisor = std::uint64_t{};
		switch (level) {
		case 0: divisor = DataBlocksPerLevel0HashTree; break;
		case 1: divisor = DataBlocksPerLevel1HashTree; break;
		case 2: divisor = DataBlocksPerLevel2HashTree; break;
		case 3: divisor = DataBlocksPerLevel3HashTree; break;
		default: std::unreachable();
		}
		return (dataPages + divisor - 1) / divisor;
	}

	using HashEntry = std::array<std::uint8_t, HashEntryLength>;

	struct HashBlock {

		std::array<HashEntry, HashEntriesPerHashBlock> Entries;
		std::uint8_t Padding0[0x10];
	};

	using DataPage = std::array<std::uint8_t, PageSize>;

	constexpr auto& PackageManifestFileName = L"appxmanifest.xml";
	constexpr auto& SegmentMetadataFileName = L"SegmentMetadata.bin";

	struct alignas(16) ExtractionContext {

		XvcRegionId RegionId{};
		std::uint32_t SegmentIndex{};
		std::uint64_t PageNumber{};
	};

	class ExtractionContextStore {
	public:

		static constexpr auto FileCookie = "bcf0b208-36b7-47a1-8756-e539c0b038ca"_Guid;
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
			auto checksum = std::span{ buffer.begin() + 0x20, buffer.end() };

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

namespace Citrine::Xbox {

	class StreamedXvcFile::Impl : public std::enable_shared_from_this<Impl> {
	public:

		Impl(Impl const&) = delete;
		auto operator=(Impl const&) = delete;

		static auto OpenFromFileAsync(File&& file) -> AsyncXvcOperationResult<StreamedXvcFile> {

			return OpenFromStreamAsync(MakeWinRTFileStream(std::move(file)));
		}

		static auto OpenFromStreamAsync(winrt::IRandomAccessStream&& stream) -> AsyncXvcOperationResult<StreamedXvcFile> {

			class ConcreteImpl : public Impl {
			public:

				ConcreteImpl(winrt::IRandomAccessStream&& stream) : Impl(std::move(stream)) {}
			};

			auto impl = std::static_pointer_cast<Impl>(std::make_shared<ConcreteImpl>(std::move(stream)));
			auto result = co_await impl->InitializeAsync();

			if (!result)
				co_return result.error();
			co_return StreamedXvcFile{ std::move(impl) };
		}

		auto PackageManifest() const noexcept -> Windows::MsixManifest const& {

			return packageManifest;
		}

		auto IsEncrypted() const noexcept -> bool {

			return encryptionEnabled;
		}

		auto GetKeyId() const noexcept -> Guid {

			if (!encryptionEnabled)
				return {};

			auto keyIdCount = std::min(std::size(xvcHeader->KeyIds), std::size_t{ xvcHeader->KeyIdCount });
			for (auto const& keyId : std::span{ xvcHeader->KeyIds, keyIdCount }) {

				if (!keyId.IsEmpty())
					return keyId;
			}

			return {};
		}

		auto ExtractAllFilesAsync(std::filesystem::path destinationDirectory, std::optional<CikEntry> cik, XvcExtractionProgressCallback progressCallback, File contextFile) -> AsyncXvcOperationResult<> try {

			if (!destinationDirectory.is_absolute())
				throw std::invalid_argument{ "Absolute path required" };

			if (encryptionEnabled && !cik)
				throw std::invalid_argument{ "Cik required" };

			if (!stream)
				co_return XvcError::StreamNotOpen;

			if (xvcSegments.empty())
				co_return {};

			auto strongSelf = shared_from_this();
			co_await winrt::resume_background();

			auto contextStore = ExtractionContextStore::Open(std::move(contextFile), packageManifest.Identity(), destinationDirectory);
			auto context = contextStore->Load();

			auto cancellationToken = co_await GetCancellationToken();
			cancellationToken.Callback([contextStore] { contextStore->Close(); });

			auto getAbsolutePath = [&rootDirectory = destinationDirectory](std::wstring_view path) {

				auto absolutePath = std::wstring{};
				absolutePath.resize_and_overwrite(rootDirectory.native().size() + 1 + path.size(), [&](wchar_t* data, std::size_t size) {

					auto out = data;
					out = std::ranges::copy(rootDirectory.native(), out).out;
					if (out[-1] != L'\\' && out[-1] != '/')
						*out++ = L'\\';
					out = std::ranges::copy(path, out).out;
					return out - data;
				});
				return std::filesystem::path{ std::move(absolutePath) };
			};

			auto createParentDirectories = [&, directories = std::unordered_set<std::wstring_view>{}](this auto& self, std::wstring_view path) -> bool {

				if (auto pos = path.find_last_of(L'\\'); pos != path.npos) {

					auto parentDirectory = std::wstring_view{ path.data(), pos };
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

			auto openFile = [&](std::wstring_view path) -> File {

				return File{ getAbsolutePath(path), FileMode::OpenAlways, FileAccess::Write };
			};

			constexpr auto isExtractableRegion = [](XvcRegionHeader const& region) static {

				auto id = region.Id;
				return id.IsValid() && (!id.IsSystem() || id == XvcRegionId::RegistrationFiles);
			};

			auto progress = XvcExtractionProgress{};
			auto currentRegion = xvcRegions.end();
			auto currentFilePosition = 0uz;

			for (auto it = xvcRegions.begin(); it != xvcRegions.end(); ++it) {

				auto& region = *it;
				if (!isExtractableRegion(region))
					continue;

				if (region.Id != context.RegionId) {

					progress.TotalBytesToProcess += region.Length;
					continue;
				}

				progress.BytesProcessed = progress.TotalBytesToProcess;
				progress.TotalBytesToProcess += region.Length;
				currentRegion = it;

				auto lastSegment = std::min(context.SegmentIndex, static_cast<std::uint32_t>(segments.size()));
				auto lastPage = std::min(context.PageNumber, BytesToPages(currentRegion->Length));

				auto segmentIndex = region.FirstSegmentIndex;
				auto pageNumber = std::uint64_t{};

				while (segmentIndex < lastSegment) {

					auto fileSize = segments[segmentIndex].FileSize;
					auto filePageCount = (fileSize > 0)
						? BytesToPages(AlignBytesToPages(fileSize))
						: 1;

					pageNumber += filePageCount;
					progress.BytesProcessed += (filePageCount * PageSize);
					++segmentIndex;
				}
				
				if (pageNumber <= lastPage) {

					currentFilePosition = ((lastPage - pageNumber) * PageSize);
					progress.BytesProcessed += currentFilePosition;
				}
			}

			if (currentRegion == xvcRegions.end()) {

				currentRegion = xvcRegions.begin();
				context = {};
			}

			constexpr auto hashBlockCacheCapacity = std::uint32_t{ 16 };
			auto hashBlockCacheBuffer = winrt::IBuffer{ nullptr };
			auto hashFunction = std::unique_ptr<Botan::HashFunction>{ nullptr };
			if (dataIntegrityEnabled) {

				hashBlockCacheBuffer = winrt::Buffer{ hashBlockCacheCapacity * sizeof(HashBlock) };
				hashFunction = Botan::HashFunction::create("SHA-256");
			}

			auto cipher = std::unique_ptr<Botan::Cipher_Mode>{ nullptr };
			if (encryptionEnabled) {

				auto key = std::array{ std::to_array(cik->DataKey), std::to_array(cik->TweakKey) };
				cipher = Botan::Cipher_Mode::create("AES-128/XTS", Botan::Cipher_Dir::Decryption);
				cipher->set_key(reinterpret_cast<std::uint8_t*>(key.data()), sizeof(key));
			}

			constexpr auto pageCacheCapacity = std::uint32_t{ 16 };
			auto pageCacheBuffer = winrt::IBuffer{ winrt::Buffer{ pageCacheCapacity * sizeof(DataPage) }};

			for (; currentRegion != xvcRegions.end(); ++currentRegion) {

				if (!isExtractableRegion(*currentRegion))
					continue;

				if (context.PageNumber == 0)
					context = { .RegionId = currentRegion->Id, .SegmentIndex = currentRegion->FirstSegmentIndex };

				auto shouldDecrypt = cik.has_value() && currentRegion->KeyIndex != 0xFFFF;
				auto tweakIv = std::array<std::uint8_t, 0x10>{};
				if (shouldDecrypt) {

					std::copy_n(reinterpret_cast<std::uint8_t const*>(&currentRegion->Id), 4, &tweakIv[4]);
					std::copy_n(reinterpret_cast<std::uint8_t const*>(&xvdHeader->VDUID), 8, &tweakIv[8]);
				}

				auto pageOffset = BytesToPages(currentRegion->Offset - userDataOffset);
				auto totalPageCount = BytesToPages(currentRegion->Length);

				auto pageCache = std::span<DataPage>{};
				auto currentPage = pageCache.end();

				auto hashBlockCache = std::span<HashBlock>{};
				auto currentHashBlock = hashBlockCache.end();
				auto currentHashEntry = decltype(HashBlock::Entries)::iterator{};

				auto regionStream = winrt::IInputStream{ nullptr };
				auto regionStreamOffset = currentRegion->Offset + (context.PageNumber * sizeof(DataPage));

				auto hashBlockStream = winrt::IInputStream{ nullptr };
				auto hashBlockStreamOffset = level0HashTreeOffset + ((pageOffset + context.PageNumber) / HashEntriesPerHashBlock * sizeof(HashBlock));

				if (auto rangeStreamProvider = stream.try_as<winrt::Citrine::IRangeStreamProvider>()) {

					regionStream = rangeStreamProvider.GetRangeStream(regionStreamOffset, currentRegion->Length);
					if (dataIntegrityEnabled) {

						auto remainingHashes = ((pageOffset + context.PageNumber) % HashEntriesPerHashBlock) + (totalPageCount - context.PageNumber);
						auto hashBlockStreamSize = ((remainingHashes / HashEntriesPerHashBlock) + static_cast<bool>(remainingHashes % HashEntriesPerHashBlock)) * sizeof(HashBlock);

						hashBlockStream = rangeStreamProvider.GetRangeStream(hashBlockStreamOffset, hashBlockStreamSize);
					}
				}
				else {

					regionStream = stream.GetInputStreamAt(regionStreamOffset);
					if (dataIntegrityEnabled) {

						hashBlockStream = stream.GetInputStreamAt(hashBlockStreamOffset);
					}
				}

				while (context.SegmentIndex < segments.size() && context.PageNumber < totalPageCount) {

					auto fileSize = segments[context.SegmentIndex].FileSize;
					auto filePath = segmentPaths[context.SegmentIndex];

					if (!createParentDirectories(filePath))
						co_return XvcError::WritingFailed;

					auto file = openFile(filePath);
					if (!file || !file.Seek(currentFilePosition))
						co_return XvcError::WritingFailed;

					do {

						contextStore->Store(context);

						if (currentPage == pageCache.end()) {

							auto remainingPages = totalPageCount - context.PageNumber;
							auto fetchCount = std::min(remainingPages, std::uint64_t{ pageCacheCapacity });

							pageCacheBuffer = co_await regionStream.ReadAsync(pageCacheBuffer, fetchCount * sizeof(DataPage), {});
							auto pageCacheReader = BufferReader{ pageCacheBuffer };

							pageCache = pageCacheReader.ReadArray<DataPage>(fetchCount);
							if (pageCache.empty())
								co_return XvcError::ReadingFailed;

							currentPage = pageCache.begin();
						}

						if (dataIntegrityEnabled) {

							if (currentHashBlock == hashBlockCache.end()) {

								auto remainingHashes = ((pageOffset + context.PageNumber) % HashEntriesPerHashBlock) + (totalPageCount - context.PageNumber);
								auto fetchCount = std::min((remainingHashes / HashEntriesPerHashBlock) + static_cast<bool>(remainingHashes % HashEntriesPerHashBlock), std::uint64_t{ hashBlockCacheCapacity });

								hashBlockCacheBuffer = co_await hashBlockStream.ReadAsync(hashBlockCacheBuffer, fetchCount * sizeof(HashBlock), {});
								auto hashBlockCacheReader = BufferReader{ hashBlockCacheBuffer };

								hashBlockCache = hashBlockCacheReader.ReadArray<HashBlock>(fetchCount);
								if (hashBlockCache.empty())
									co_return XvcError::ReadingFailed;

								currentHashBlock = hashBlockCache.begin();
								currentHashEntry = currentHashBlock->Entries.begin() + ((pageOffset + context.PageNumber) % HashEntriesPerHashBlock);
							}

							auto digest = TrivialArray<std::uint8_t, 0x20>{};
							hashFunction->update(*currentPage);
							hashFunction->final(digest);

							if (!std::ranges::equal(std::span{ digest.data(), hashLength }, std::span{ currentHashEntry->data(), hashLength }))
								co_return XvcError::DataIntegrityViolation;

							if (shouldDecrypt) {

								std::copy_n(currentHashEntry->data() + hashLength, 4, &tweakIv[0]);
							}

							if (++currentHashEntry == currentHashBlock->Entries.end()) {

								if (++currentHashBlock != hashBlockCache.end())
									currentHashEntry = currentHashBlock->Entries.begin();
							}
						}

						if (shouldDecrypt) {

							cipher->start(tweakIv);
							cipher->process(*currentPage);
						}

						auto fileSection = std::span{ currentPage->data(), std::min(fileSize - currentFilePosition, std::uint64_t{ PageSize }) };
						if (!file.Write(fileSection))
							co_return XvcError::WritingFailed;

						currentFilePosition += fileSection.size();
						progress.BytesProcessed += PageSize;

						if (progressCallback)
							progressCallback(progress);

						++currentPage;
						++context.PageNumber;

					} while (currentFilePosition < fileSize);

					if (!file.Truncate())
						co_return XvcError::WritingFailed;

					++context.SegmentIndex;
					currentFilePosition = 0;
				}

				context = {};
				currentFilePosition = 0;
			}

			co_return{};
		}
		catch (winrt::hresult_error const&) {

			co_return XvcError::ReadingFailed;
		}

		auto DetachStream() noexcept -> winrt::IRandomAccessStream {

			return std::move(stream);
		}

	protected:

		Impl(winrt::IRandomAccessStream&& stream)

			: stream(std::move(stream))
		{}

		auto InitializeAsync() -> AsyncXvcOperationResult<> {

			return ParseAsync();
		}

		auto ParseAsync() -> AsyncXvcOperationResult<> {

			if (!stream)
				co_return XvcError::StreamNotOpen;

			auto strongSelf = shared_from_this();
			co_await winrt::resume_background();

			if (auto result = co_await ParseXvdHeaderAsync(); !result)
				co_return result.error();

			if (xvdHeader->UserDataLength == 0)
				co_return XvcError::UnsupportedFormat;

			if (auto result = co_await ParseUserDataAsync(); !result)
				co_return result.error();

			if (!userDataPackageContents.contains(PackageManifestFileName))
				co_return XvcError::UnsupportedFormat;

			if (auto result = ParsePackageManifest(); !result)
				co_return result.error();

			if (!userDataPackageContents.contains(SegmentMetadataFileName))
				co_return XvcError::UnsupportedFormat;

			if (auto result = ParseSegmentMetadata(); !result)
				co_return result.error();

			if (xvdHeader->XvcDataLength == 0)
				co_return XvcError::UnsupportedFormat;

			if (auto result = co_await ParseXvcDataAsync(); !result)
				co_return result.error();

			co_return {};
		}

		auto ParseXvdHeaderAsync() -> AsyncXvcOperationResult<> try {

			stream.Seek(0);
			xvdHeaderBuffer = co_await stream.ReadAsync(winrt::Buffer{ sizeof(XvdHeader) }, sizeof(XvdHeader), {});
			auto xvdHeaderReader = BufferReader{ xvdHeaderBuffer };

			xvdHeader = xvdHeaderReader.Read<XvdHeader>();
			if (!xvdHeader || xvdHeader->Cookie != XvdMagic)
				co_return XvcError::ParsingFailed;

			dataIntegrityEnabled = (xvdHeader->VolumeFlags & XvdVolumeFlags::DataIntegrityDisabled) != XvdVolumeFlags::DataIntegrityDisabled;
			resiliencyEnabled = (xvdHeader->VolumeFlags & XvdVolumeFlags::ResiliencyEnabled) == XvdVolumeFlags::ResiliencyEnabled;
			encryptionEnabled = (xvdHeader->VolumeFlags & XvdVolumeFlags::EncryptionDisabled) != XvdVolumeFlags::EncryptionDisabled;
			hashLength = encryptionEnabled ? 0x14 : 0x18;

			if (dataIntegrityEnabled) {

				hashedPages = BytesToPages(AlignBytesToPages(xvdHeader->DriveSize));
				hashedPages += BytesToPages(AlignBytesToPages(xvdHeader->UserDataLength));
				hashedPages += BytesToPages(AlignBytesToPages(xvdHeader->XvcDataLength));
				hashedPages += BytesToPages(AlignBytesToPages(xvdHeader->DynamicHeaderLength));
			}

			embeddedXvdOffset = sizeof(XvdHeader) + UnusedSpace;
			mutableXvcDataOffset = embeddedXvdOffset + AlignBytesToPages(xvdHeader->EmbeddedXvdLength);
			level3HashTreeOffset = mutableXvcDataOffset + PagesToBytes(xvdHeader->MutableXvcDataPageCount);
			level2HashTreeOffset = level3HashTreeOffset + PagesToBytes(NumberOfHashPagesForLevel(hashedPages, 3));
			level1HashTreeOffset = level2HashTreeOffset + PagesToBytes(NumberOfHashPagesForLevel(hashedPages, 2));
			level0HashTreeOffset = level1HashTreeOffset + PagesToBytes(NumberOfHashPagesForLevel(hashedPages, 1));
			userDataOffset = level0HashTreeOffset + PagesToBytes(NumberOfHashPagesForLevel(hashedPages, 0));
			xvcHeaderOffset = userDataOffset + AlignBytesToPages(xvdHeader->UserDataLength);
			blockAllocationTableOffset = xvcHeaderOffset + AlignBytesToPages(xvdHeader->XvcDataLength);
			dataOffset = blockAllocationTableOffset + AlignBytesToPages(xvdHeader->DynamicHeaderLength);

			co_return {};
		}
		catch (winrt::hresult_error const&) {

			co_return XvcError::ReadingFailed;
		}

		auto ParseUserDataAsync() -> AsyncXvcOperationResult<> try {

			stream.Seek(userDataOffset);
			userDataBuffer = co_await stream.ReadAsync(winrt::Buffer{ xvdHeader->UserDataLength }, xvdHeader->UserDataLength, {});
			auto userDataReader = BufferReader{ userDataBuffer };

			userDataHeader = userDataReader.Read<XvdUserDataHeader>();
			if (!userDataHeader)
				co_return XvcError::ParsingFailed;

			if (userDataHeader->DataType != XvdUserDataType::PackageFiles)
				co_return XvcError::UnsupportedFormat;

			userDataReader.Seek(userDataHeader->HeaderLength);
			userDataPackageFilesHeader = userDataReader.Read<XvdUserDataPackageFilesHeader>();
			if (!userDataPackageFilesHeader || userDataPackageFilesHeader->EntryCount >= 0x7FFFFFFF)
				co_return XvcError::ParsingFailed;

			userDataPackageFiles = userDataReader.ReadArray<XvdUserDataPackageFile>(userDataPackageFilesHeader->EntryCount);
			if (!userDataPackageFiles.data())
				co_return XvcError::ParsingFailed;

			auto userDataContentSpan = std::span{ userDataBuffer.data() + userDataHeader->HeaderLength, userDataBuffer.Length() - userDataHeader->HeaderLength };
			for (auto const& packageFile : userDataPackageFiles) {

				auto offset = packageFile.Offset;
				auto fileSize = packageFile.FileSize;

				if (offset + fileSize > userDataContentSpan.size())
					co_return XvcError::ParsingFailed;

				auto fileContentBegin = userDataContentSpan.begin() + offset;
				auto fileContentEnd = fileContentBegin + fileSize;

				auto filePath = std::wstring_view{ packageFile.FilePath, std::ranges::find(packageFile.FilePath, L'\0') };
				userDataPackageContents.try_emplace(filePath, fileContentBegin, fileContentEnd);
			}

			co_return{};
		}
		catch (winrt::hresult_error const&) {

			co_return XvcError::ReadingFailed;
		}

		auto ParsePackageManifest() -> XvcOperationResult<> {

			auto result = Windows::MsixManifest::OpenFromBuffer(auto{ userDataPackageContents.find(PackageManifestFileName)->second });
			if (!result)
				return XvcError::InvalidPackageManifest;

			packageManifest = std::move(*result);
			return {};
		}

		auto ParseSegmentMetadata() -> XvcOperationResult<> {

			auto& segmentMetadata = userDataPackageContents.find(SegmentMetadataFileName)->second;
			if (segmentMetadata.size() > sizeof(XvdSegmentMetadataHeader) && (std::bit_cast<std::uintptr_t>(segmentMetadata.data()) & 1) != 0) {

				// Sacrifice null termination for alignment
				std::copy_backward(segmentMetadata.begin(), segmentMetadata.end() - 1, segmentMetadata.end());
				segmentMetadata = { segmentMetadata.data() + 1, segmentMetadata.size() - 1 };
			}
			auto segmentMetadataReader = BufferReader{ segmentMetadata };

			segmentMetadataHeader = segmentMetadataReader.Read<XvdSegmentMetadataHeader>();
			if (!segmentMetadataHeader || segmentMetadataHeader->SegmentCount >= 0x7FFFFFFF)
				return XvcError::ParsingFailed;

			segments = segmentMetadataReader.ReadArray<XvdSegmentMetadataSegment>(segmentMetadataHeader->SegmentCount);
			if (!segments.data())
				return XvcError::ParsingFailed;

			segmentPaths.reserve(segmentMetadataHeader->SegmentCount);
			auto segmentPathsOffset = segmentMetadataHeader->HeaderLength + (sizeof(XvdSegmentMetadataSegment) * segmentMetadataHeader->SegmentCount);

			for (auto const& segment : segments) {

				segmentMetadataReader.Seek(segmentPathsOffset + segment.PathOffset);
				auto path = segmentMetadataReader.ReadArray<wchar_t>(segment.PathLength);
				if (!path.data())
					return XvcError::ParsingFailed;

				segmentPaths.emplace_back(path);
			}

			return {};
		}

		auto ParseXvcDataAsync() -> AsyncXvcOperationResult<> try {

			stream.Seek(xvcHeaderOffset);
			xvcDataBuffer = co_await stream.ReadAsync(winrt::Buffer{ xvdHeader->XvcDataLength }, xvdHeader->XvcDataLength, {});
			auto xvcDataReader = BufferReader{ xvcDataBuffer };

			xvcHeader = xvcDataReader.Read<XvcHeader>();
			if (!xvcHeader)
				co_return XvcError::ParsingFailed;

			if (xvcHeader->Version >= 1) {

				xvcRegions = xvcDataReader.ReadArray<XvcRegionHeader>(xvcHeader->RegionCount);
				if (!xvcRegions.data())
					co_return XvcError::ParsingFailed;

				xvcSegments = xvcDataReader.ReadArray<XvcSegment>(xvcHeader->SegmentCount);
				if (!xvcSegments.data())
					co_return XvcError::ParsingFailed;

				if (xvcHeader->Version >= 2) {

					xvcRegionSpecifiers = xvcDataReader.ReadArray<XvcRegionSpecifier>(xvcHeader->RegionSpecifierCount);
					if (!xvcRegionSpecifiers.data())
						co_return XvcError::ParsingFailed;
				}
			}

			co_return {};
		}
		catch (winrt::hresult_error const&) {

			co_return XvcError::ReadingFailed;
		}

		winrt::IRandomAccessStream stream{ nullptr };

		winrt::IBuffer xvdHeaderBuffer{ nullptr };
		XvdHeader* xvdHeader{ nullptr };

		bool dataIntegrityEnabled{};
		bool resiliencyEnabled{};
		bool encryptionEnabled{};
		std::uint8_t hashLength{};
		std::uint64_t hashedPages{};
		std::uint64_t embeddedXvdOffset{};
		std::uint64_t mutableXvcDataOffset{};
		std::uint64_t level3HashTreeOffset{};
		std::uint64_t level2HashTreeOffset{};
		std::uint64_t level1HashTreeOffset{};
		std::uint64_t level0HashTreeOffset{};
		std::uint64_t userDataOffset{};
		std::uint64_t xvcHeaderOffset{};
		std::uint64_t blockAllocationTableOffset{};
		std::uint64_t dataOffset{};

		winrt::IBuffer userDataBuffer{ nullptr };
		XvdUserDataHeader* userDataHeader{ nullptr };
		XvdUserDataPackageFilesHeader* userDataPackageFilesHeader{ nullptr };
		std::span<XvdUserDataPackageFile> userDataPackageFiles;
		std::map<std::wstring_view, std::span<std::uint8_t>> userDataPackageContents;

		Windows::MsixManifest packageManifest{ nullptr };

		XvdSegmentMetadataHeader* segmentMetadataHeader{ nullptr };
		std::span<XvdSegmentMetadataSegment> segments;
		std::vector<std::wstring_view> segmentPaths;

		winrt::IBuffer xvcDataBuffer{ nullptr };
		XvcHeader* xvcHeader{ nullptr };
		std::span<XvcRegionHeader> xvcRegions;
		std::span<XvcSegment> xvcSegments;
		std::span<XvcRegionSpecifier> xvcRegionSpecifiers;
	};

	StreamedXvcFile::StreamedXvcFile(std::shared_ptr<Impl>&& impl) noexcept

		: impl(std::move(impl))
	{}

	auto StreamedXvcFile::operator=(std::nullptr_t) noexcept -> StreamedXvcFile& {

		impl.reset();
		return *this;
	}

	auto StreamedXvcFile::OpenFromFileAsync(File&& file) -> AsyncXvcOperationResult<StreamedXvcFile> {

		return Impl::OpenFromFileAsync(std::move(file));
	}

	auto StreamedXvcFile::OpenFromStreamAsync(winrt::IRandomAccessStream&& stream) -> AsyncXvcOperationResult<StreamedXvcFile> {

		return Impl::OpenFromStreamAsync(std::move(stream));
	}

	auto StreamedXvcFile::PackageManifest() const noexcept -> Windows::MsixManifest const& {

		return impl->PackageManifest();
	}

	auto StreamedXvcFile::IsEncrypted() const noexcept -> bool {

		return impl->IsEncrypted();
	}

	auto StreamedXvcFile::GetKeyId() const noexcept -> Guid {

		return impl->GetKeyId();
	}

	auto StreamedXvcFile::ExtractAllFilesAsync(std::filesystem::path destination, std::optional<CikEntry> cik, XvcExtractionProgressCallback progressCallback, File contextFile) -> AsyncXvcOperationResult<> {

		return impl->ExtractAllFilesAsync(std::move(destination), std::move(cik), std::move(progressCallback), std::move(contextFile));
	}

	StreamedXvcFile::operator bool() const noexcept {

		return static_cast<bool>(impl);
	}

	auto StreamedXvcFile::Release() noexcept -> void {

		impl.reset();
	}

	auto StreamedXvcFile::Stream() && noexcept -> winrt::Windows::Storage::Streams::IRandomAccessStream {

		return impl->DetachStream();
	}

	auto StreamedXvcFile::swap(StreamedXvcFile& other) noexcept -> void {

		impl.swap(other.impl);
	}
}