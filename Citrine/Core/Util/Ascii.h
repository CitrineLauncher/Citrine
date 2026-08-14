#pragma once

#include "Core/Util/StringLiteral.h"

#include <array>
#include <bit>
#include <string_view>
#include <stdexcept>

namespace Citrine {

	struct Ascii {

		static constexpr auto ToUpperTable = [] static {

			auto table = std::array<std::uint8_t, 256>{};
			for (auto i = 0uz; i < table.size(); ++i)
				table[i] = static_cast<std::uint8_t>(i - std::size_t{ i >= 'a' && i <= 'z' } * 32);
			return table;
		}();

		static constexpr auto ToLowerTable = [] static {

			auto table = std::array<std::uint8_t, 256>{};
			for (auto i = 0uz; i < table.size(); ++i)
				table[i] = static_cast<std::uint8_t>(i + std::size_t{ i >= 'A' && i <= 'Z' } * 32);
			return table;
		}();

		template<typename CharT>
		static constexpr auto ToUpper(CharT ch) noexcept -> CharT {

			if constexpr (sizeof(CharT) > sizeof(std::uint8_t)) {

				auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
				return uch < 0x80 ? CharT{ ToUpperTable[uch] } : ch;
			}
			else {

				return std::bit_cast<CharT>(ToUpperTable[std::bit_cast<std::uint8_t>(ch)]);
			}
		}

		template<typename CharT>
		static constexpr auto ToLower(CharT ch) noexcept -> CharT {

			if constexpr (sizeof(CharT) > sizeof(std::uint8_t)) {

				auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
				return uch < 0x80 ? CharT{ ToLowerTable[uch] } : ch;
			}
			else {

				return std::bit_cast<CharT>(ToLowerTable[std::bit_cast<std::uint8_t>(ch)]);
			}
		}

		static constexpr auto CaseInsensitiveEquals(std::string_view left, std::string_view right) noexcept -> bool {

			constexpr auto toLower = [](char ch) static { return ToLower(ch); };
			return std::ranges::equal(left, right, {}, toLower, toLower);
		}

		static constexpr auto CaseInsensitiveEquals(std::wstring_view left, std::wstring_view right) noexcept -> bool {

			constexpr auto toLower = [](wchar_t ch) static { return ToLower(ch); };
			return std::ranges::equal(left, right, {}, toLower, toLower);
		}
	};

	class AsciiMatcherBase {
	protected:

		template<StringLiteral Expr>
		static consteval auto BuildBitMap() -> std::array<bool, 256> {

			auto it = Expr.data();
			auto const end = it + Expr.size();

			auto next = [&] {

				if (it < end && *it == '\\') ++it;
				if (it == end || *it < 0x0 || *it > 0x7f) throw std::invalid_argument{ "" };
				return *it++;
			};

			auto match = [&](char ch) {

				auto result = it < end && *it == ch;
				if (result) ++it;
				return result;
			};

			auto bitMap = std::array<bool, 256>{};

			while (it < end) {

				auto lower = next();
				if (match('-')) {

					auto upper = next();
					if (lower >= upper)
						throw std::invalid_argument{ "" };

					while (lower <= upper)
						bitMap[lower++] = true;
				}
				else {

					bitMap[lower] = true;
				}
			}

			return bitMap;
		}

		template<std::array<bool, 256> V>
		static constexpr auto MakeStatic = V;
	};

	template<StringLiteral Expr>
	class AsciiMatcher : AsciiMatcherBase {
	public:

		template<typename CharT>
		static constexpr auto operator()(CharT ch) noexcept -> bool {

			if constexpr (sizeof(CharT) > sizeof(std::uint8_t)) {

				auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
				return uch < 0x80 ? CharT{ MakeStatic<BuildBitMap<Expr>()>[uch] } : ch;
			}
			else {

				return MakeStatic<BuildBitMap<Expr>()>[std::bit_cast<std::uint8_t>(ch)];
			}
		}
	};
}