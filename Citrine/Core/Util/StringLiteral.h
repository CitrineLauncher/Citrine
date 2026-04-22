#pragma once

#include "Core/Util/Concepts.h"

#include <string_view>
#include <algorithm>
#include <stdexcept>
#include <format>

namespace Citrine {

	template<typename CharT, size_t N> requires (N > 0)
	struct BasicStringLiteral {

		using value_type = CharT;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = const_pointer;
		using const_iterator = const_pointer;

		constexpr BasicStringLiteral() noexcept = default;

		constexpr BasicStringLiteral(CharT const (&str)[N]) noexcept {

			std::copy_n(str, N, Data);
		}

		constexpr BasicStringLiteral(BasicStringLiteral const&) noexcept = default;
		constexpr auto operator=(BasicStringLiteral const&) noexcept -> BasicStringLiteral& = default;

		constexpr auto operator[](size_type index) const noexcept -> const_reference {

			return Data[index];
		}

		constexpr auto data() const noexcept -> const_pointer {

			return Data;
		}

		constexpr auto begin() const noexcept -> const_iterator {

			return Data;
		}

		constexpr auto end() const noexcept -> const_iterator {

			return Data + Size;
		}

		constexpr auto empty() const noexcept -> bool {

			return Size == 0;
		}

		constexpr auto size() const noexcept -> size_type {

			return Size;
		}

		constexpr operator std::basic_string_view<CharT>() const {

			return { Data, Size };
		}

		constexpr auto operator<=>(BasicStringLiteral const&) const -> std::strong_ordering = default;

		static constexpr auto Size = N - 1;
		CharT Data[N];
	};

	template<std::size_t N>
	struct StringLiteral : BasicStringLiteral<char, N> {

		constexpr StringLiteral(BasicStringLiteral<char, N> const& other) noexcept

			: BasicStringLiteral<char, N>::BasicStringLiteral(other)
		{}

		using BasicStringLiteral<char, N>::BasicStringLiteral;
		using BasicStringLiteral<char, N>::operator=;
	};

	template<std::size_t N>
	StringLiteral(char const(&)[N]) -> StringLiteral<N>;

	template<std::size_t N>
	struct WStringLiteral : BasicStringLiteral<wchar_t, N> {

		constexpr WStringLiteral(BasicStringLiteral<wchar_t, N> const& other) noexcept

			: BasicStringLiteral<wchar_t, N>::BasicStringLiteral(other)
		{}

		using BasicStringLiteral<wchar_t, N>::BasicStringLiteral;
		using BasicStringLiteral<wchar_t, N>::operator=;
	};

	template<std::size_t N>
	WStringLiteral(wchar_t const(&)[N]) -> WStringLiteral<N>;

	template<typename To, typename From, std::size_t N>
	consteval auto StringLiteralCast(From const(&from)[N]) -> BasicStringLiteral<To, N> {

		auto result = BasicStringLiteral<To, N>{};
		for (auto i = 0uz; i < N; ++i) {

			auto ch = from[i];
			if (ch < 0x0 || ch > 0x7F)
				throw std::invalid_argument{ "StringLiteralCast only supports ASCII string literals" };
			result.Data[i] = static_cast<To>(ch);
		}
		return result;
	}

	template<typename To, typename From, std::size_t N>
	consteval auto StringLiteralCast(BasicStringLiteral<From, N> const& from) -> BasicStringLiteral<To, N> {

		return StringLiteralCast<To>(from.Data);
	}
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT, std::size_t N>
	struct formatter<::Citrine::BasicStringLiteral<CharT, N>, CharT> : formatter<basic_string_view<CharT>, CharT> {};

	template<std::size_t N>
	struct formatter<::Citrine::StringLiteral<N>, char> : formatter<string_view, char> {};

	template<std::size_t N>
	struct formatter<::Citrine::WStringLiteral<N>, wchar_t> : formatter<wstring_view, wchar_t> {};
}