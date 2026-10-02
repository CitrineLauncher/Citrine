#pragma once

#include <string_view>
#include <array>

namespace Citrine {

	template<typename CharT>
	inline constexpr auto Radix100Table = std::array<CharT, 200>{

		'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
		'0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
		'1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
		'1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
		'2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
		'2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
		'3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
		'3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
		'4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
		'4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
		'5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
		'5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
		'6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
		'6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
		'7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
		'7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
		'8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
		'8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
		'9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
		'9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
	};

	template<typename CharT = char>
	class FormatInteger {
	public:

		explicit constexpr FormatInteger(std::int32_t value) noexcept : str(FormatSigned(value)) {}
		explicit constexpr FormatInteger(std::int64_t value) noexcept : str(FormatSigned(value)) {}
		explicit constexpr FormatInteger(std::uint32_t value) noexcept : str(FormatUnsigned(value)) {}
		explicit constexpr FormatInteger(std::uint64_t value) noexcept : str(FormatUnsigned(value)) {}

		FormatInteger(FormatInteger const&) = delete;
		auto operator=(FormatInteger const&) = delete;

		constexpr auto begin() const noexcept -> CharT const* {

			return str;
		}

		constexpr auto end() const noexcept -> CharT const* {

			return buffer + BufferSize;
		}

		constexpr auto operator[](std::size_t index) const noexcept -> CharT const& {

			return str[index];
		}

		constexpr auto data() const noexcept -> CharT const* {

			return str;
		}

		constexpr auto size() const noexcept -> std::size_t {

			return buffer + BufferSize - str;
		}

		constexpr operator std::basic_string_view<CharT>() const noexcept {

			return { data(), size() };
		}

	private:

		template<typename T>
		constexpr auto FormatUnsigned(T value) noexcept -> CharT* {

			auto out = buffer + BufferSize;
			while (value >= 100) {

				out -= 2;
				std::copy_n(&Radix100Table<CharT>[value % 100 * 2], 2, out);
				value /= 100;
			}

			if (value >= 10) {

				out -= 2;
				std::copy_n(&Radix100Table<CharT>[value * 2], 2, out);
				return out;
			}

			*--out = CharT{ '0' } + static_cast<CharT>(value);
			return out;
		}

		template<typename T>
		constexpr auto FormatSigned(T value) noexcept -> CharT* {

			auto absValue = static_cast<std::make_unsigned_t<T>>(value);
			if (value < 0) absValue = 0 - absValue;

			auto begin = FormatUnsigned(absValue);
			if (value < 0) *--begin = '-';
			return begin;
		}

		static constexpr auto BufferSize = 20; // i64(sign + 19 digits) | u64(20 digits)

		CharT buffer[BufferSize];
		CharT const* str;
	};
}