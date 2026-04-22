#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/Math.h"

#include <type_traits>
#include <limits>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <robuffer.h>

namespace Citrine {

	template<typename T>
		requires IsAnyOf<typename T::value_type, char, std::uint8_t>
	struct BasicWinRTBuffer : winrt::implements<
		BasicWinRTBuffer<T>,
		winrt::Windows::Storage::Streams::IBuffer,
		::Windows::Storage::Streams::IBufferByteAccess>
	{
		BasicWinRTBuffer(T&& underlying) noexcept(std::is_nothrow_move_constructible_v<T>)

			: myData(reinterpret_cast<std::uint8_t*>(underlying.data()))
			, mySize(SaturatingCast<std::uint32_t>(underlying.size()))
			, myCapacity(mySize)
			, underlying(std::move(underlying))
		{}

		auto Length() const noexcept -> std::uint32_t {

			return mySize;
		}

		auto Length(std::uint32_t size) -> void {

			if (size > myCapacity)
				throw winrt::hresult_invalid_argument{};

			mySize = size;
		}

		auto Capacity() const noexcept -> std::uint32_t {

			return myCapacity;
		}

		auto __stdcall Buffer(std::uint8_t** value) noexcept -> ::HRESULT final override {

			*value = myData;
			return S_OK;
		}

	private:

		std::uint8_t* myData{};
		std::uint32_t mySize{};
		std::uint32_t myCapacity{};
		T underlying;
	};

	template<typename T> requires std::is_rvalue_reference_v<T&&>
	auto MakeWinRTBuffer(T&& underlying) -> winrt::Windows::Storage::Streams::IBuffer {

		return winrt::make<BasicWinRTBuffer<T>>(std::move(underlying));
	}
}