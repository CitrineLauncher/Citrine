#include "pch.h"
#include "WinRTFileStream.h"

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Storage::Streams;
}

namespace Citrine {
	
	BasicWinRTFileStream::BasicWinRTFileStream(File&& file) noexcept
	
		: file(std::move(file))
	{}

	auto BasicWinRTFileStream::Size() -> std::uint64_t {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		auto currentSize = std::uint64_t{};
		if (!file.GetSize(currentSize))
			throw winrt::hresult_error{ static_cast<winrt::hresult>(file.LastError()) };

		return currentSize;
	}

	auto BasicWinRTFileStream::Size(std::uint64_t newSize) -> void {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		if (!file.Resize(newSize))
			throw winrt::hresult_error{ static_cast<winrt::hresult>(file.LastError()) };
	}

	auto BasicWinRTFileStream::Position() -> std::uint64_t {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		auto currentPos = std::uint64_t{};
		file.GetPosition(currentPos);
		return currentPos;
	}

	auto BasicWinRTFileStream::Seek(std::uint64_t newPos) -> void {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		file.Seek(newPos);
	}

	auto BasicWinRTFileStream::CanRead() const noexcept -> bool {

		return file.CanRead();
	}

	auto BasicWinRTFileStream::ReadAsync(winrt::IBuffer const& buffer, std::uint32_t count, winrt::InputStreamOptions) -> winrt::IAsyncOperationWithProgress<winrt::IBuffer, std::uint32_t> {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		if (buffer.Capacity() < count)
			throw winrt::hresult_invalid_argument{};

		auto actualCount = std::uint64_t{};
		if (!file.Read({ buffer.data(), count }, actualCount))
			throw winrt::hresult_error{ static_cast<winrt::hresult>(file.LastError()) };

		buffer.Length(static_cast<std::uint32_t>(actualCount));
		co_return buffer;
	}

	auto BasicWinRTFileStream::CanWrite() const noexcept -> bool {

		return file.CanWrite();
	}

	auto BasicWinRTFileStream::WriteAsync(winrt::IBuffer const& buffer) -> winrt::IAsyncOperationWithProgress<std::uint32_t, std::uint32_t> {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		auto count = buffer.Length();
		if (!file.Write({ buffer.data(), count }))
			throw winrt::hresult_error{ static_cast<winrt::hresult>(file.LastError()) };

		co_return count;
	}

	auto BasicWinRTFileStream::FlushAsync() -> winrt::IAsyncOperation<bool> {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		co_return file.Flush();
	}

	auto BasicWinRTFileStream::GetInputStreamAt(std::uint64_t startPos) -> winrt::IInputStream {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		auto clonedStream = winrt::make_self<BasicWinRTFileStream>(file.Clone());
		clonedStream->Seek(startPos);
		return *clonedStream;
	}

	auto BasicWinRTFileStream::GetOutputStreamAt(std::uint64_t startPos) -> winrt::IOutputStream {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		auto clonedStream = winrt::make_self<BasicWinRTFileStream>(file.Clone());
		clonedStream->Seek(startPos);
		return *clonedStream;
	}

	auto BasicWinRTFileStream::CloneStream() -> winrt::IRandomAccessStream {

		if (!file)
			throw winrt::hresult_error{ RO_E_CLOSED };

		return winrt::make<BasicWinRTFileStream>(file.Clone());
	}

	auto BasicWinRTFileStream::Close() noexcept -> void {

		file.Close();
	}
	
	auto MakeWinRTFileStream(File&& file) -> winrt::IRandomAccessStream {

		return winrt::make<BasicWinRTFileStream>(std::move(file));
	}
}