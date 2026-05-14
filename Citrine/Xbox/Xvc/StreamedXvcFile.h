#pragma once

#include "Common.h"
#include "Xbox/Keys/CikEntry.h"

#include "Core/Util/Guid.h"
#include "Core/IO/File.h"
#include "Windows/Msix/MsixManifest.h"

#include <memory>
#include <filesystem>
#include <optional>
#include <functional>

#include <winrt/Windows.Storage.Streams.h>

namespace Citrine::Xbox {

	struct XvcExtractionProgress {

		std::uint64_t BytesProcessed{};
		std::uint64_t TotalBytesToProcess{};
	};

	using XvcExtractionProgressCallback = std::move_only_function<auto(XvcExtractionProgress const&) -> void>;

	class StreamedXvcFile {
	public:

		constexpr StreamedXvcFile(std::nullptr_t) noexcept {};
		auto operator=(std::nullptr_t) noexcept -> StreamedXvcFile&;

		StreamedXvcFile(StreamedXvcFile const&) = delete;
		auto operator=(StreamedXvcFile const&) = delete;

		StreamedXvcFile(StreamedXvcFile&&) noexcept = default;
		auto operator=(StreamedXvcFile&&) noexcept -> StreamedXvcFile& = default;

		static auto OpenFromFileAsync(File&& file) -> AsyncXvcOperationResult<StreamedXvcFile>;
		static auto OpenFromStreamAsync(winrt::Windows::Storage::Streams::IRandomAccessStream&& stream) -> AsyncXvcOperationResult<StreamedXvcFile>;

		auto PackageManifest() const noexcept -> Windows::MsixManifest const&;

		auto IsEncrypted() const noexcept -> bool;
		auto GetKeyId() const noexcept -> Guid;

		auto ExtractAllFilesAsync(std::filesystem::path destinationDirectory, std::optional<CikEntry> cik, XvcExtractionProgressCallback progressCallback = nullptr, File contextFile = {}) -> AsyncXvcOperationResult<>;

		operator bool() const noexcept;
		auto Release() noexcept -> void;

		auto Stream() && noexcept -> winrt::Windows::Storage::Streams::IRandomAccessStream;
		auto swap(StreamedXvcFile& other) noexcept -> void;

	private:

		class Impl;

		StreamedXvcFile(std::shared_ptr<Impl>&& impl) noexcept;

		std::shared_ptr<Impl> impl{ nullptr };
	};
}