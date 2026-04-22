#pragma once

#include "Core/IO/File.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

namespace Citrine {
	
	struct BasicWinRTFileStream : winrt::implements<
		BasicWinRTFileStream,
		winrt::Windows::Storage::Streams::IRandomAccessStream,
		winrt::Windows::Storage::Streams::IInputStream,
		winrt::Windows::Storage::Streams::IOutputStream,
		winrt::Windows::Foundation::IClosable>
	{
		BasicWinRTFileStream(File&& file) noexcept;

		auto Size() -> std::uint64_t;
		auto Size(std::uint64_t newSize) -> void;

		auto Position() -> std::uint64_t;
		auto Seek(std::uint64_t newPos) -> void;

		auto CanRead() const noexcept -> bool;
		auto ReadAsync(
			winrt::Windows::Storage::Streams::IBuffer const& buffer,
			std::uint32_t count,
			winrt::Windows::Storage::Streams::InputStreamOptions
		) -> winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Windows::Storage::Streams::IBuffer, std::uint32_t>;

		auto CanWrite() const noexcept -> bool;
		auto WriteAsync(
			winrt::Windows::Storage::Streams::IBuffer const& buffer
		) -> winrt::Windows::Foundation::IAsyncOperationWithProgress<std::uint32_t, std::uint32_t>;
		auto FlushAsync() -> winrt::Windows::Foundation::IAsyncOperation<bool>;

		auto GetInputStreamAt(std::uint64_t) -> winrt::Windows::Storage::Streams::IInputStream;
		auto GetOutputStreamAt(std::uint64_t) -> winrt::Windows::Storage::Streams::IOutputStream;
		auto CloneStream() -> winrt::Windows::Storage::Streams::IRandomAccessStream;

		auto Close() noexcept -> void;

	private:

		File file;
	};

	auto MakeWinRTFileStream(File&& file) -> winrt::Windows::Storage::Streams::IRandomAccessStream;
	
}