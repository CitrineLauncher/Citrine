#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/Ascii.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Unicode/Utf.h"

#include <string_view>
#include <algorithm>
#include <format>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine {

	class LanguageSubTag {
	public:

		constexpr LanguageSubTag() noexcept = default;

		constexpr LanguageSubTag(LanguageSubTag const&) noexcept = default;
		constexpr auto operator=(LanguageSubTag const&) noexcept -> LanguageSubTag& = default;

		constexpr auto IsEmpty() const noexcept -> bool {

			auto sizeByte = std::bit_cast<std::uint8_t>(myData[7]);
			return sizeByte == 0;
		}

		constexpr operator std::string_view() const noexcept {

			auto sizeByte = std::bit_cast<std::uint8_t>(myData[7]);
			return { myData, sizeByte >= 8 ? 8uz : sizeByte };
		}

		constexpr auto operator==(this std::string_view left, std::string_view right) noexcept -> bool {

			return left == right;
		}

		constexpr auto operator<=>(this std::string_view left, std::string_view right) noexcept -> std::strong_ordering {

			return left <=> right;
		}

	private:

		friend class LanguageTag;

		constexpr LanguageSubTag(char const* data, std::uint8_t size) noexcept {

			std::copy_n(data, size, myData);
			if (size <= 7)
				myData[7] = std::bit_cast<char>(size);
		}

		char myData[8]{};
	};

	class LanguageTag {
	public:

		static constexpr auto MaxSize = 17uz; // language(8) + script(5) + region(4)

		constexpr LanguageTag() noexcept = default;

		constexpr LanguageTag(std::string_view str) noexcept {
		
			auto truncated = str.size() > MaxSize;
			auto maxCount = truncated ? MaxSize : str.size();

			std::copy_n(str.data(), maxCount, myData);
			mySize = static_cast<std::uint8_t>(maxCount);
			wellFormed = !truncated && Parse({ myData, mySize }, subTagInfo);
		}

		constexpr LanguageTag(auto const& str) noexcept
		
			: LanguageTag(std::string_view{ str })
		{}

		constexpr LanguageTag(LanguageTag const&) noexcept = default;
		constexpr auto operator=(LanguageTag const&) noexcept -> LanguageTag& = default;

		constexpr auto Language() const noexcept -> LanguageSubTag {

			auto languageSize = subTagInfo.LanguageSize;
			return { myData, languageSize };
		}

		constexpr auto Script() const noexcept -> LanguageSubTag {

			auto scriptPosition = subTagInfo.ScriptPosition;
			auto scriptSize = subTagInfo.ScriptSize;
			return { myData + scriptPosition, scriptSize };
		}

		constexpr auto Region() const noexcept -> LanguageSubTag {

			auto regionPosition = subTagInfo.RegionPosition;
			auto regionSize = subTagInfo.RegionSize;
			return { myData + regionPosition, regionSize };
		}

		constexpr auto IsEmpty() const noexcept -> bool {

			return mySize == 0;
		}

		constexpr auto IsWellFormed() const noexcept -> bool {

			return wellFormed;
		}

		constexpr operator std::string_view() const noexcept {

			return { myData, mySize };
		}

		constexpr auto operator==(this std::string_view left, std::string_view right) noexcept -> bool {

			return left == right;
		}

		constexpr auto operator<=>(this std::string_view left, std::string_view right) noexcept -> std::strong_ordering {

			return left <=> right;
		}

	private:

		struct SubTagInfo {

			std::uint8_t LanguageSize{};
			std::uint8_t ScriptPosition{};
			std::uint8_t ScriptSize{};
			std::uint8_t RegionPosition{};
			std::uint8_t RegionSize{};
		};

		static constexpr auto Parse(std::string_view str, SubTagInfo& subTagInfo) noexcept -> bool {
		
			if (str.empty() || str.size() > MaxSize)
				return false;

			auto const begin = str.data();
			auto const end = begin + str.size();

			auto it = begin;

			constexpr auto isAlpha = AsciiMatcher<"A-Za-z">{};
			constexpr auto isDec = [](char ch) static { return DigitFromChar(ch) < 10; };

			auto advanceWhile = [&](auto pred) {

				while (it < end) {

					if (!pred(*it))
						return;
					++it;
				}
			};

			//language
			{
				advanceWhile(isAlpha);
				auto tagSize = it - begin;

				if (tagSize < 2 || tagSize == 4 || tagSize > 8)
					return false;

				subTagInfo.LanguageSize = static_cast<std::uint8_t>(tagSize);
			}

			if (it == end)
				return true;

			if (*it++ != '-')
				return false;

			//script and region
			{
				auto oldPos = it;
				auto tagPosition = it - begin;
				advanceWhile(isAlpha);
				auto tagSize = it - begin - tagPosition;

				if (tagSize == 4) {

					subTagInfo.ScriptPosition = static_cast<std::uint8_t>(tagPosition);
					subTagInfo.ScriptSize = static_cast<std::uint8_t>(tagSize);

					if (it < end && *it++ != '-')
						return false;

					oldPos = it;
					tagPosition = it - begin;
					advanceWhile(isAlpha);
					tagSize = it - begin - tagPosition;
				}

				if (tagSize == 0) {

					it = oldPos;
					tagPosition = it - begin;
					advanceWhile(isDec);
					tagSize = it - begin - tagPosition;

					if (tagSize != 3)
						return false;
				}
				else {

					if (tagSize != 2)
						return false;
				}

				subTagInfo.RegionPosition = static_cast<std::uint8_t>(tagPosition);
				subTagInfo.RegionSize = static_cast<std::uint8_t>(tagSize);
			}

			return it == end;
		}

		char myData[MaxSize]{};
		std::uint8_t mySize{};
		SubTagInfo subTagInfo{};
		bool wellFormed{};
	};

	template<typename T>
	concept IsLanguageTagType = IsAnyOf<std::remove_cv_t<T>, LanguageTag, LanguageSubTag>;
}

namespace std {

	template<::Citrine::IsLanguageTagType T, ::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<T, CharT> : formatter<basic_string_view<CharT>, CharT> {

		auto format(T const& tag, auto& ctx) const -> auto {

			using namespace ::Citrine;

			if constexpr (std::same_as<CharT, wchar_t>)
				return formatter<wstring_view, wchar_t>::format(ToUtf16(tag), ctx);
			else
				return formatter<string_view, char>::format(tag, ctx);
		}
	};
}

namespace glz {

	template<>
	struct from<JSON, ::Citrine::LanguageTag>
	{
		template<auto Opts>
		static auto op(::Citrine::LanguageTag& languageTag, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			languageTag = LanguageTag{ str };
			if (!languageTag.IsEmpty() && !languageTag.IsWellFormed())
				ctx.error = error_code::parse_error;
		}
	};

	template<>
	struct to<JSON, ::Citrine::LanguageTag>
	{
		template<auto Opts>
		static auto op(::Citrine::LanguageTag const& languageTag, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			serialize<JSON>::op<Opts>(std::string_view{ languageTag }, args...);
		}
	};
}