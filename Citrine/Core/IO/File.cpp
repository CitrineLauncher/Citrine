#include "pch.h"
#include "File.h"

#include "Core/Util/Math.h"
#include "Core/Util/VariableSizedType.h"
#include "Core/Util/Guid.h"

#include <format>

namespace {

	constexpr auto MakeOverlapped(std::uint64_t offset, ::HANDLE handle = nullptr) noexcept -> ::OVERLAPPED {

		return { 0, 0, { static_cast<::DWORD>(offset & 0xFFFFFFFF), static_cast<::DWORD>(offset >> 32) }, handle };
	}
}

namespace Citrine {

	File::File(std::filesystem::path const& path, FileMode mode, FileAccess access, FileShare share, FileOptions options) noexcept {

		Open(path, mode, access, share, options);
	}

	File::File(File&& other) noexcept

		: handle(std::exchange(other.handle, nullptr))
		, position(std::exchange(other.position, {}))
		, permissions(std::exchange(other.permissions, {}))
		, lastError(std::exchange(other.lastError, {}))
	{}

	auto File::operator=(File&& other) noexcept -> File& {

		File{ std::move(other) }.swap(*this);
		return *this;
	}

	auto File::Open(std::filesystem::path const& path, FileMode mode, FileAccess access, FileShare share, FileOptions options) noexcept -> bool {

		if (IsOpen())
			return false;

		auto fileHandle = ::CreateFileW(
			path.c_str(),
			std::to_underlying(access),
			std::to_underlying(share),
			nullptr,
			std::to_underlying(mode),
			std::to_underlying(options),
			nullptr
		);

		if (fileHandle == INVALID_HANDLE_VALUE) {

			lastError = ::GetLastError();
			return false;
		}

		handle = fileHandle;
		permissions = access;
		return true;
	}

	auto File::GetSize(std::uint64_t& currentSize) noexcept -> bool {

		currentSize = 0;
		if (!IsOpen())
			return false;

		auto value = ::LARGE_INTEGER{ .QuadPart = {} };

		if (!::GetFileSizeEx(handle, &value)) {

			lastError = ::GetLastError();
			return false;
		}

		currentSize = static_cast<std::uint64_t>(value.QuadPart);
		return true;
	}

	auto File::Resize(std::uint64_t newSize) noexcept -> bool {

		if (!IsOpen())
			return false;

		auto info = ::FILE_END_OF_FILE_INFO{};
		info.EndOfFile.QuadPart = static_cast<::LONGLONG>(newSize);

		if (!::SetFileInformationByHandle(handle, FileEndOfFileInfo, &info, sizeof(info))) {

			lastError = ::GetLastError();
			return false;
		}
		return true;
	}

	auto File::Truncate() noexcept -> bool {

		auto size = std::uint64_t{};
		if (!GetSize(size))
			return false;

		if (position < size)
			return Resize(position);
		return true;
	}

	auto File::CanRead() const noexcept -> bool {

		return (permissions & FileAccess::Read) == FileAccess::Read;
	}

	auto File::Read(std::span<std::uint8_t> buffer, std::size_t& bytesRead) noexcept -> bool {

		auto& totalBytesRead = bytesRead = 0;
		auto currentBytesRead = ::DWORD{};
		auto result = false;

		if (!IsOpen())
			return false;

		do {

			auto overlapped = MakeOverlapped(position);
			result = ::ReadFile(handle, buffer.data() + totalBytesRead, SaturatingCast<::DWORD>(buffer.size() - totalBytesRead), &currentBytesRead, &overlapped);
			position += currentBytesRead;
			totalBytesRead += currentBytesRead;

		} while (result && totalBytesRead < buffer.size() && currentBytesRead > 0);
		
		if (!result)
			lastError = ::GetLastError();
		return result;
	}

	auto File::Read(std::span<char> buffer, std::size_t& charsRead) noexcept -> bool {

		return Read({ reinterpret_cast<std::uint8_t*>(buffer.data()), buffer.size() }, charsRead);
	}

	auto File::Read(BasicBuffer& buffer) noexcept -> bool {

		auto bytesRead = 0uz;
		auto result = Read({ buffer.data(), buffer.capacity() }, bytesRead);
		buffer.resize(bytesRead);

		return result;
	}

	auto File::ReadToEnd(std::vector<std::uint8_t>& vec) -> bool {

		auto size = std::uint64_t{};
		if (!IsOpen() || !GetSize(size)) {

			vec.clear();
			return false;
		}

		vec.resize(static_cast<std::size_t>(size));
		auto bytesRead = 0uz;
		auto result = Read(vec, bytesRead);
		vec.resize(bytesRead);

		return result;
	}

	auto File::ReadToEnd(std::string& str) -> bool {

		str.clear();
		auto size = std::uint64_t{};
		if (!IsOpen() || !GetSize(size))
			return false;

		auto result = false;
		str.resize_and_overwrite(static_cast<std::size_t>(size), [&](char* data, std::size_t size) {

			auto charsRead = 0uz;
			result = Read({ reinterpret_cast<std::uint8_t*>(data), size }, charsRead);
			return charsRead;
		});

		return result;
	}

	auto File::CanWrite() const noexcept -> bool {

		return (permissions & FileAccess::Write) == FileAccess::Write;
	}

	auto File::Write(std::span<std::uint8_t const> buffer) noexcept -> bool {

		auto totalBytesWritten = 0uz;
		auto currentBytesWritten = ::DWORD{};
		auto result = false;

		if (!IsOpen())
			return false;

		do {

			auto overlapped = MakeOverlapped(position);
			result = ::WriteFile(handle, buffer.data() + totalBytesWritten, SaturatingCast<::DWORD>(buffer.size() - totalBytesWritten), &currentBytesWritten, &overlapped);
			position += currentBytesWritten;
			totalBytesWritten += currentBytesWritten;

		} while (result && totalBytesWritten < buffer.size());

		if (!result)
			lastError = ::GetLastError();
		return result;
	}

	auto File::Write(std::string_view str) noexcept -> bool {

		return Write({ reinterpret_cast<std::uint8_t const*>(str.data()), str.size() });
	}

	auto File::Flush() noexcept -> bool {

		if (!IsOpen())
			return false;

		if (!::FlushFileBuffers(handle)) {

			lastError = ::GetLastError();
			return false;
		}
		return true;
	}

	auto File::GetPosition(std::uint64_t& currentPos) noexcept -> bool {

		currentPos = 0;
		if (!IsOpen())
			return false;

		currentPos = position;
		return true;
	}

	auto File::Seek(std::uint64_t newPos) noexcept -> bool {

		if (!IsOpen())
			return false;

		position = newPos;
		return true;
	}

	auto File::SeekToBegin() noexcept -> bool {

		if (!IsOpen())
			return false;

		position = 0;
		return true;
	}

	auto File::SeekToEnd() noexcept -> bool {

		auto size = std::uint64_t{};
		if (!GetSize(size))
			return false;

		position = size;
		return true;
	}

	auto File::Rename(std::filesystem::path const& newName, bool replaceExisting) -> bool {

		if (!IsOpen())
			return false;

		using Info = ::FILE_RENAME_INFO;
		auto info = VariableSizedType<Info>{
			{
				.ReplaceIfExists = replaceExisting,
				.RootDirectory = nullptr
			},
			newName.native(),
			&Info::FileName,
			&Info::FileNameLength
		};

		if (!::SetFileInformationByHandle(handle, FileRenameInfo, info.Get(), static_cast<::DWORD>(info.Size()))) {

			lastError = ::GetLastError();
			return false;
		}
		return true;
	}

	auto File::Clone() const noexcept -> File {

		auto file = File{};
		if (!IsOpen())
			return file;

		auto proc = ::GetCurrentProcess();

		::DuplicateHandle(proc, handle, proc, &file.handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
		file.permissions = permissions;
		file.lastError = lastError;
		return file;
	}

	auto File::Delete() noexcept -> bool {

		if (!IsOpen())
			return false;

		auto info = ::FILE_DISPOSITION_INFO{ .DeleteFileW = true };

		if (!::SetFileInformationByHandle(handle, FileDispositionInfo, &info, sizeof(info))) {

			lastError = ::GetLastError();
			return false;
		}
		return true;
	}

	auto File::IsOpen() const noexcept -> bool {

		return static_cast<bool>(handle);
	}

	File::operator bool() const noexcept {

		return IsOpen();
	}

	auto File::Close() noexcept -> void {

		if (IsOpen()) {

			::CloseHandle(handle);
			handle = nullptr;
			position = {};
			permissions = {};
			lastError = {};
		}
	}

	auto File::NativeHandle() const noexcept -> HandleType {

		return handle;
	}

	auto File::LastError() const noexcept -> std::uint32_t {

		return lastError;
	}

	auto File::Detach() noexcept -> HandleType {

		auto fileHandle = std::exchange(handle, nullptr);
		position = {};
		permissions = {};
		lastError = {};
		return fileHandle;
	}

	auto File::swap(File& other) noexcept -> void {

		std::swap(handle, other.handle);
		std::swap(position, other.position);
		std::swap(permissions, other.permissions);
		std::swap(lastError, other.lastError);
	}

	File::~File() noexcept {

		Close();
	}

	auto ReadFile(std::filesystem::path const& path, std::vector<std::uint8_t>& vec) -> bool {

		auto file = File{ path, FileMode::OpenExisting, FileAccess::Read };
		return file.ReadToEnd(vec);
	}

	auto ReadFile(std::filesystem::path const& path, std::string& str) -> bool {
#
		auto file = File{ path, FileMode::OpenExisting, FileAccess::Read };
		return file.ReadToEnd(str);
	}

	auto WriteFile(std::filesystem::path const& path, std::span<std::uint8_t const> buffer) noexcept -> bool {

		auto file = File{ path, FileMode::OpenAlways, FileAccess::Write };
		return file.Write(buffer) && file.Truncate();
	}

	auto WriteFile(std::filesystem::path const& path, std::string_view str) noexcept -> bool {

		auto file = File{ path, FileMode::OpenAlways, FileAccess::Write };
		return file.Write(str) && file.Truncate();
	}

	auto CreateTempFile() -> File {

		auto ec = std::error_code{};
		auto fileName = Guid::Create().Format(L"N") + L".tmp";
		auto file = File{};

		if (auto path = std::filesystem::temp_directory_path(ec) / std::move(fileName); !ec) {

			file.Open(
				path,
				FileMode::CreateNew,
				FileAccess::ReadWrite,
				FileShare::None,
				FileOptions::Temporary | FileOptions::DeleteOnClose
			);
		}
		return file;
	}

	auto RotateFile(std::filesystem::path const& path, std::uint8_t maxFiles) -> bool {

		auto splitted = [path = std::wstring_view{ path.native() }] -> std::pair<std::wstring_view, std::wstring_view> {

			if (auto pos = path.find_last_of(L"\\/."); pos != path.npos && path[pos] == '.' && pos < path.size() - 1)
				return { path.substr(0, pos), path.substr(pos) };
			return { path, {} };
		}();

		auto calculatePath = [&](std::uint8_t index) -> std::filesystem::path {

			if (index == 0)
				return path;
			return std::format(L"{}.{}{}", splitted.first, index, splitted.second);
		};

		auto ec = std::error_code{};
		auto targetPath = calculatePath(maxFiles);

		for (auto i = maxFiles; i > 0; --i) {

			auto sourcePath = calculatePath(i - 1);
			if (std::filesystem::rename(sourcePath, targetPath, ec); ec && ec.value() != ERROR_FILE_NOT_FOUND) return false;
			targetPath = std::move(sourcePath);
		}
		return true;
	}
}