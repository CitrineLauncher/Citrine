#pragma once

#include "Core/Util/Concepts.h"

#include <type_traits>
#include <algorithm>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

namespace Citrine {

	template<typename T>
		requires IsAnyOf<typename T::value_type, char, std::uint8_t>
	struct BasicWinRTMemoryStream : winrt::implements<
		BasicWinRTMemoryStream<T>,
		winrt::Windows::Storage::Streams::IRandomAccessStream,
		winrt::Windows::Storage::Streams::IInputStream,
		winrt::Windows::Storage::Streams::IOutputStream,
		winrt::Windows::Foundation::IClosable>
	{
		static constexpr auto Resizable = requires(T underlying) { underlying.resize(0); };

		BasicWinRTMemoryStream(T&& underlying) noexcept(std::is_nothrow_move_constructible_v<T>)

			: underlying(std::move(underlying))
			, position(0)
		{}

		auto Size() const noexcept -> std::uint64_t {

			return underlying.size();
		}

		auto Size(std::uint64_t newSize) -> void requires Resizable {

			if constexpr (sizeof(std::uint64_t) > sizeof(std::size_t)) {

				if (newSize > std::numeric_limits<std::size_t>::max())
					throw winrt::hresult_error{ static_cast<winrt::hresult>(0x80030008) }; // STG_E_INSUFFICIENTMEMORY
			}

			try {

				underlying.resize(static_cast<std::size_t>(newSize));
			}
			catch (...) {

				throw winrt::hresult_error{ static_cast<winrt::hresult>(0x80030008) }; // STG_E_INSUFFICIENTMEMORY
			}
		}

		auto Size(std::uint64_t) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto Position() const noexcept -> std::uint64_t {

			return position;
		}

		auto Seek(std::uint64_t newPos) noexcept -> void {

			position = newPos;
		}

		auto CanRead() const noexcept -> bool {

			return true;
		}

		auto ReadAsync(
			winrt::Windows::Storage::Streams::IBuffer const& buffer,
			std::uint32_t count,
			winrt::Windows::Storage::Streams::InputStreamOptions
		) -> winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Windows::Storage::Streams::IBuffer, std::uint32_t>{

			auto const startPos = std::min(position, std::uint64_t{ underlying.size() });
			auto const actualCount = std::min(std::uint64_t{ count }, std::uint64_t{ underlying.size() } - startPos);

			buffer.Length(static_cast<std::uint32_t>(actualCount));
			std::copy_n(underlying.data() + startPos, actualCount, buffer.data());
			position += actualCount;

			co_return buffer;
		}

		auto CanWrite() const noexcept -> bool {

			return true;
		}

		auto WriteAsync(
			winrt::Windows::Storage::Streams::IBuffer const& buffer
		) -> winrt::Windows::Foundation::IAsyncOperationWithProgress<std::uint32_t, std::uint32_t> requires Resizable {

			auto const count = buffer.Length();
			auto const requiredSize = position + count;

			if (underlying.size() < requiredSize)
				Size(requiredSize);

			std::copy_n(buffer.data(), count, underlying.data() + position);
			position += count;

			co_return count;
		}

		auto WriteAsync(
			winrt::Windows::Storage::Streams::IBuffer const& buffer
		) -> winrt::Windows::Foundation::IAsyncOperationWithProgress<std::uint32_t, std::uint32_t> {

			auto const startPos = std::min(position, std::uint64_t{ underlying.size() });
			auto const actualCount = std::min(std::uint64_t{ buffer.Length() }, std::uint64_t{ underlying.size() } - startPos);

			std::copy_n(buffer.data(), actualCount, underlying.data() + startPos);
			position += actualCount;

			co_return actualCount;
		}

		auto FlushAsync() -> winrt::Windows::Foundation::IAsyncOperation<bool> {

			co_return true;
		}

		auto GetInputStreamAt(std::uint64_t) -> winrt::Windows::Storage::Streams::IInputStream {

			throw winrt::hresult_not_implemented{};
		}

		auto GetOutputStreamAt(std::uint64_t) -> winrt::Windows::Storage::Streams::IOutputStream {

			throw winrt::hresult_not_implemented{};
		}

		auto CloneStream() -> winrt::Windows::Storage::Streams::IRandomAccessStream {

			throw winrt::hresult_not_implemented{};
		}

		auto Close() noexcept -> void {
		
		}

	private:

		T underlying;
		std::uint64_t position{};
	};

	template<typename T> requires std::is_rvalue_reference_v<T&&>
	auto MakeWinRTMemoryStream(T&& underlying) -> winrt::Windows::Storage::Streams::IRandomAccessStream {

		return winrt::make<BasicWinRTMemoryStream<T>>(std::move(underlying));
	}
}