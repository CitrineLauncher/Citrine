#pragma once

#include <array>
#include <type_traits>
#include <limits>
#include <bit>

namespace Citrine {

	inline constexpr auto DigitFromByte = std::array<std::uint8_t, 256>{

		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255,
		255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
		25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255,
		255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
		25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	};

	template<typename CharT>
	auto constexpr DigitFromChar(CharT ch) noexcept -> std::uint8_t {

		if constexpr (sizeof(CharT) > sizeof(std::uint8_t)) {

			auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
			if (uch > 0x7f) uch = 0;
			return DigitFromByte[uch];
		}
		else {

			return DigitFromByte[std::bit_cast<std::uint8_t>(ch)];
		}
	}

	enum struct ParseIntegerError {

		None,
		NoDigits,
		Overflow,
		Underflow
	};

	template<typename CharT>
	struct ParseIntegerResult {

		explicit constexpr operator bool() const noexcept {

			return Error == ParseIntegerError::None;
		}

		constexpr auto operator==(ParseIntegerError error) const noexcept -> bool {

			return Error == error;
		}

		constexpr auto operator<=>(ParseIntegerError error) const noexcept -> std::strong_ordering {

			return Error <=> error;
		}

		CharT const* Ptr{};
		ParseIntegerError Error{};
	};

	template<typename CharT, std::integral T>
	constexpr auto ParseInteger(CharT const* first, CharT const* last, T& value) -> ParseIntegerResult<CharT> {

		auto it = first;
		auto negative = false;

		if constexpr (std::is_signed_v<T>) {

			if (it < last && *it == '-') {

				++it;
				negative = true;
			}
		}

		using Limits = std::numeric_limits<T>;
		using UnsignedT = std::conditional_t<sizeof(T) <= 4, std::uint32_t, std::uint64_t>;

		constexpr auto maxValue = static_cast<UnsignedT>(Limits::max());
		auto riskyValue = maxValue / 10;
		auto maxDigit = maxValue % 10;
		auto overflowError = ParseIntegerError::Overflow;

		if constexpr (std::is_signed_v<T>) {

			if (negative) {

				constexpr auto absMinValue = static_cast<UnsignedT>(Limits::max()) + 1;
				riskyValue = absMinValue / 10;
				maxDigit = absMinValue % 10;
				overflowError = ParseIntegerError::Underflow;
			}
		}

		auto num = UnsignedT{};

		constexpr auto digits10 = static_cast<std::ptrdiff_t>(Limits::digits10);
		for (auto const dst = it + std::min(last - it, digits10); it < dst; ++it) {

			auto digit = DigitFromChar(*it);
			if (digit >= 10) { last = it; break; }
			num = num * 10 + static_cast<UnsignedT>(digit);
		}

		auto overflowed = false;
		for (; it < last; ++it) {

			auto digit = DigitFromChar(*it);
			if (digit >= 10) break;

			if (num < riskyValue || (num == riskyValue && digit <= maxDigit))
				num = num * 10 + static_cast<UnsignedT>(digit);
			else
				overflowed = true;
		}

		if (overflowed)
			return { it, overflowError };

		if (it - first == static_cast<std::ptrdiff_t>(negative))
			return { it, ParseIntegerError::NoDigits };

		if constexpr (std::is_signed_v<T>) {

			if (negative)
				num = static_cast<UnsignedT>(0 - num);
		}

		value = static_cast<T>(num);
		return { it, ParseIntegerError::None };
	}
}