#pragma once

#include "Core/Util/Concepts.h"
#include "Core/IO/Buffer.h"

#include <filesystem>
#include <utility>
#include <span>
#include <vector>
#include <string>

namespace Citrine {

	enum struct FileMode : std::uint32_t {

		CreateNew = 1,
		CreateAlways = 2,
		OpenExisting = 3,
		OpenAlways = 4,
		TruncateExisting = 5
	};

	enum struct FileAccess : std::uint32_t {

		Read = 0x80000000,
		Write = 0x40000000,
		ReadWrite = Read | Write,
		Delete = 0x00010000
	};

	enum struct FileShare : std::uint32_t {

		None = 0,
		Read = 1,
		Write = 2,
		ReadWrite = Read | Write,
		Delete = 4
	};

	enum struct FileOptions : std::uint32_t {

		None = 0x00000080,
		Temporary = 0x00000100,
		Encrypted = 0x00004000,
		DeleteOnClose = 0x04000000,
		SequentialScan = 0x08000000,
		RandomAccess = 0x10000000,
		Asynchronous = 0x40000000,
		WriteThrough = 0x80000000
	};

	template<IsAnyOf<FileAccess, FileShare, FileOptions> T>
	constexpr auto operator|(T left, T right) noexcept -> T {

		return T{ std::to_underlying(left) | std::to_underlying(right) };
	}

	template<IsAnyOf<FileAccess, FileShare, FileOptions> T>
	constexpr auto operator&(T left, T right) noexcept -> T {

		return T{ std::to_underlying(left) & std::to_underlying(right) };
	}

	class File {
	public:

		using HandleType = void*;

		constexpr File() noexcept = default;

		File(
			std::filesystem::path const& path,
			FileMode mode,
			FileAccess access,
			FileShare share = FileShare::ReadWrite,
			FileOptions options = FileOptions::None
		) noexcept;

		File(File const&) = delete;
		auto operator=(File const&) = delete;

		File(File&& other) noexcept;
		auto operator=(File&& other) noexcept -> File&;

		auto Open(
			std::filesystem::path const& path,
			FileMode mode,
			FileAccess access,
			FileShare share = FileShare::ReadWrite,
			FileOptions options = FileOptions::None
		) noexcept -> bool;

		auto GetSize(std::uint64_t& currentSize) noexcept -> bool;
		auto Resize(std::uint64_t newSize) noexcept -> bool;
		auto Truncate() noexcept -> bool;

		auto CanRead() const noexcept -> bool;
		auto Read(std::span<std::uint8_t> buffer, std::size_t& bytesRead) noexcept -> bool;
		auto Read(std::span<char> buffer, std::size_t& charsRead) noexcept -> bool;
		auto Read(BasicBuffer& buffer) noexcept -> bool;
		auto ReadToEnd(std::vector<std::uint8_t>& vec) -> bool;
		auto ReadToEnd(std::string& str) -> bool;

		auto CanWrite() const noexcept -> bool;
		auto Write(std::span<std::uint8_t const> buffer) noexcept -> bool;
		auto Write(std::string_view str) noexcept -> bool;
		auto Flush() noexcept -> bool;

		auto GetPosition(std::uint64_t& currentPos) noexcept -> bool;
		auto Seek(std::uint64_t newPos) noexcept -> bool;
		auto SeekToBegin() noexcept -> bool;
		auto SeekToEnd() noexcept -> bool;

		auto Rename(std::filesystem::path const& newName, bool replaceExisting = false) -> bool;
		auto Clone() const noexcept -> File;
		auto Delete() noexcept -> bool;

		auto IsOpen() const noexcept -> bool;
		explicit operator bool() const noexcept;
		auto Close() noexcept -> void;

		auto NativeHandle() const noexcept -> HandleType;
		auto LastError() const noexcept -> std::uint32_t;
		auto Detach() noexcept -> HandleType;
		auto swap(File& other) noexcept -> void;

		~File() noexcept;

	private:

		HandleType handle{ nullptr };
		std::uint64_t position{};
		FileAccess permissions{};
		std::uint32_t lastError{};
	};

	auto ReadFile(std::filesystem::path const& path, std::vector<std::uint8_t>& vec) -> bool;
	auto ReadFile(std::filesystem::path const& path, std::string& str) -> bool;

	auto WriteFile(std::filesystem::path const& path, std::span<std::uint8_t const> buffer) noexcept -> bool;
	auto WriteFile(std::filesystem::path const& path, std::string_view str) noexcept -> bool;

	auto CreateTempFile() -> File;
	auto RotateFile(std::filesystem::path const& path, std::uint8_t maxFiles) -> bool;
}

