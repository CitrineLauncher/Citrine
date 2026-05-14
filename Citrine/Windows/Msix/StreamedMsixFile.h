#pragma once

#include "Common.h"
#include "MsixManifest.h"

#include "Core/IO/File.h"

#include <memory>
#include <filesystem>
#include <functional>

#include <winrt/Windows.Storage.Streams.h>

namespace Citrine::Windows {

	struct MsixExtractionProgress {

		std::uint64_t BytesProcessed{};
		std::uint64_t TotalBytesToProcess{};
	};

	using MsixExtractionProgressCallback = std::move_only_function<auto(MsixExtractionProgress const&) -> void>;

	class StreamedMsixFile {
	public:

		constexpr StreamedMsixFile(std::nullptr_t) noexcept {};
		auto operator=(std::nullptr_t) noexcept -> StreamedMsixFile&;

		StreamedMsixFile(StreamedMsixFile const&) = delete;
		auto operator=(StreamedMsixFile const&) = delete;

		StreamedMsixFile(StreamedMsixFile&&) noexcept = default;
		auto operator=(StreamedMsixFile&&) noexcept -> StreamedMsixFile& = default;

		static auto OpenFromFileAsync(File&& file) -> AsyncMsixOperationResult<StreamedMsixFile>;
		static auto OpenFromStreamAsync(winrt::Windows::Storage::Streams::IRandomAccessStream&& stream) -> AsyncMsixOperationResult<StreamedMsixFile>;

		auto PackageManifest() const noexcept -> MsixManifest const&;

		auto ExtractAllFilesAsync(std::filesystem::path destinationDirectory, MsixExtractionProgressCallback progressCallback = nullptr, File contextFile = {}) -> AsyncMsixOperationResult<>;

		operator bool() const noexcept;
		auto Release() noexcept -> void;

		auto Stream() && noexcept -> winrt::Windows::Storage::Streams::IRandomAccessStream;
		auto swap(StreamedMsixFile& other) noexcept -> void;

	private:

		class Impl;

		StreamedMsixFile(std::shared_ptr<Impl>&& impl) noexcept;

		std::shared_ptr<Impl> impl{ nullptr };
	};
}