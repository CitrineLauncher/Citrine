#pragma once

#include "Core/Util/Concepts.h"

#include <string_view>
#include <algorithm>

namespace Citrine {

	template<StandardLayoutType T>
	class VariableSizedType {
	public:

		template<IsAnyOf<char, wchar_t> CharT>
		constexpr VariableSizedType(T const& obj, auto const& str, CharT (T::* dataField)[1]) {

			Initialize(obj, std::basic_string_view<CharT>{ str }, dataField);
		}

		template<IsAnyOf<char, wchar_t> CharT, typename SizeT>
		constexpr VariableSizedType(T const& obj, auto const& str, CharT (T::* dataField)[1], SizeT T::* dataSizeField) {

			Initialize(obj, std::basic_string_view<CharT>{ str }, dataField, dataSizeField);
		}

		VariableSizedType(VariableSizedType const&) = delete;
		auto operator=(VariableSizedType const&) = delete;

		constexpr VariableSizedType(VariableSizedType&& other) noexcept

			: ptr(std::exchange(other.ptr, nullptr))
			, size(other.size)
		{}

		constexpr auto operator=(VariableSizedType&& other) noexcept -> VariableSizedType& {

			VariableSizedType{ std::move(other) }.swap(*this);
			return *this;
		}

		template<typename Self>
		constexpr auto Get(this Self&& self) noexcept -> auto {

			return std::addressof(std::forward_like<Self&>(*self.ptr));
		}

		constexpr auto Size() const noexcept -> std::size_t {

			return size;
		}

		template<typename Self>
		constexpr auto operator*(this Self&& self) noexcept -> decltype(auto) {

			return std::forward_like<Self&>(*self.ptr);
		}

		template<typename Self>
		constexpr auto operator->(this Self&& self) noexcept -> auto {

			return std::addressof(std::forward_like<Self&>(*self.ptr));
		}

		constexpr auto swap(VariableSizedType& other) noexcept -> void {

			std::swap(ptr, other.ptr);
			std::swap(size, other.size);
		}

		constexpr ~VariableSizedType() noexcept {

			Release();
		}

	private:

		template<typename CharT>
		constexpr auto Initialize(T const& obj, std::basic_string_view<CharT> str, CharT (T::* dataField)[1]) -> void {

			static_assert((__STDCPP_DEFAULT_NEW_ALIGNMENT__ & (alignof(T) - 1)) == 0);

			size = sizeof(T) + (str.size() * sizeof(CharT));
			ptr = ::new (::new std::byte[size]) T(obj);

			for (auto i = 1uz; i < str.size() + 1; ++i)
				::new (&(ptr->*dataField)[i]) CharT;

			std::ranges::copy(str, ptr->*dataField);
			(ptr->*dataField)[str.size()] = '\0';
		}

		template<typename CharT, typename SizeT>
		constexpr auto Initialize(T const& obj, std::basic_string_view<CharT> str, CharT (T::* dataField)[1], SizeT T::* dataSizeField) -> void {

			Initialize(obj, str, dataField);
			ptr->*dataSizeField = static_cast<SizeT>(str.size());
		}

		constexpr auto Release() noexcept -> void {

			if (ptr) Destroy();
		}

		constexpr auto Destroy() noexcept -> void {

			ptr->~T();
			::delete[] reinterpret_cast<std::byte*>(ptr);
		}

		T* ptr;
		std::size_t size;
	};
}