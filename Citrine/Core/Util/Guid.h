#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/Hash.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Codec/Base16.h"

#include <bit>
#include <string>
#include <optional>
#include <format>

#include <guiddef.h>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine {

	template<typename CharT>
	class BasicGuidFormatString;

	using GuidFormatString = BasicGuidFormatString<char>;
	using WGuidFormatString = BasicGuidFormatString<wchar_t>;

	struct Guid {

		static constexpr auto Parse(std::string_view str) noexcept -> std::optional<Guid>;
		static constexpr auto Parse(std::string_view str, Guid& value) noexcept -> bool;

		static constexpr auto Parse(std::wstring_view str) noexcept -> std::optional<Guid>;
		static constexpr auto Parse(std::wstring_view str, Guid& value) noexcept -> bool;

		static auto Create() noexcept -> Guid;

		template<IsAnyOf<char, wchar_t> CharT = char>
		constexpr auto Format() const -> std::basic_string<CharT>;
		constexpr auto Format(GuidFormatString fmt) const -> std::string;
		constexpr auto Format(WGuidFormatString fmt) const -> std::wstring;

		template<IsAnyOf<char, wchar_t> CharT>
		constexpr auto FormatTo(std::basic_string<CharT>& str) const -> void;
		constexpr auto FormatTo(std::string& str, GuidFormatString fmt) const -> void;
		constexpr auto FormatTo(std::wstring& str, WGuidFormatString fmt) const -> void;

		constexpr auto IsEmpty() const noexcept -> bool {

			return *this == Guid{};
		}

		template<IsAnyOf<char, wchar_t> CharT>
		explicit constexpr operator std::basic_string<CharT>() const {

			return Format<CharT>();
		}

		constexpr operator ::GUID() const noexcept {

			return std::bit_cast<::GUID>(*this);
		}

		constexpr auto operator<=>(Guid const&) const noexcept -> std::strong_ordering = default;

		std::uint32_t Data1;
		std::uint16_t Data2;
		std::uint16_t Data3;
		std::uint8_t  Data4[8];
	};

	consteval auto operator ""_Guid(char const* str, std::size_t size) -> Guid;

	struct GuidParser {

		template<std::same_as<char> CharT>
		static constexpr auto Parse(std::basic_string_view<CharT> str, Guid& value) -> bool {

			if (str.size() < 32 || str.size() > 38)
				return false;

			if (str.size() == 38 && ((str[0] == '{' && str[37] == '}') || (str[0] == '(' && str[37] == ')'))) {

				str.remove_prefix(1);
				str.remove_suffix(1);
			}

			auto bytes = std::array<std::uint8_t, 16>{};
			auto result = false;

			if (str.size() == 36) {

				result =
					Base16::Decode({ &str[0], 8 }, &bytes[0]) &&
					str[8] == '-' &&
					Base16::Decode({ &str[9], 4 }, &bytes[4]) &&
					str[13] == '-' &&
					Base16::Decode({ &str[14], 4 }, &bytes[6]) &&
					str[18] == '-' &&
					Base16::Decode({ &str[19], 4 }, &bytes[8]) &&
					str[23] == '-' &&
					Base16::Decode({ &str[24], 12 }, &bytes[10]);
			}
			else if (str.size() == 32) {

				result = Base16::Decode(str, bytes.data());
			}

			if (result) {

				value = std::bit_cast<Guid>(bytes);
				if constexpr (std::endian::native == std::endian::little) {

					value.Data1 = std::byteswap(value.Data1);
					value.Data2 = std::byteswap(value.Data2);
					value.Data3 = std::byteswap(value.Data3);
				}
			}
			return result;
		}

		template<std::same_as<wchar_t> CharT>
		static constexpr auto Parse(std::basic_string_view<CharT> str, Guid& value) -> bool {

			if (str.size() < 32 || str.size() > 38)
				return false;

			auto buffer = TrivialArray<char, 38>{};
			for (auto out = buffer.data(); auto ch : str) {

				auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
				if (uch > 0x7f) uch = 0;
				*out++ = static_cast<char>(ch);
			}

			return Parse<char>({ buffer.data(), str.size() }, value);
		}
	};

	class GuidFormatBase {
	protected:

		enum struct FormatSpecifier : std::uint8_t {

			N, // 00000000000000000000000000000000
			D, // 00000000-0000-0000-0000-000000000000
			B, // {00000000-0000-0000-0000-000000000000}
			P  // (00000000-0000-0000-0000-000000000000)
		};
	};

	template<typename CharT>
	class BasicGuidFormatString : GuidFormatBase {
	public:

		template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicGuidFormatString(T const& str) {

			*this = Parse(str);
		}

		template<typename CharT>
		explicit constexpr BasicGuidFormatString(BasicGuidFormatString<CharT> fmt)

			: formatSpecifier(fmt.formatSpecifier)
			, uppercase(fmt.uppercase)
			, formattedSize(fmt.formattedSize)
		{}

		constexpr BasicGuidFormatString(BasicGuidFormatString const&) noexcept = default;
		constexpr auto operator=(BasicGuidFormatString const&) noexcept -> BasicGuidFormatString& = default;

		static constexpr auto Parse(std::basic_string_view<CharT> str) -> BasicGuidFormatString {

			if (str.size() > 1)
				throw std::format_error{ "Too many format specifiers" };

			auto fmt = BasicGuidFormatString{};
			auto ch = str.size() > 0
				? str.front()
				: 'd';

			if (ch == 'N' || ch == 'n') {

				fmt.formatSpecifier = FormatSpecifier::N;
				fmt.formattedSize = 32;
			}
			else if (ch == 'D' || ch == 'd') {

				fmt.formatSpecifier = FormatSpecifier::D;
				fmt.formattedSize = 36;
			}
			else if (ch == 'B' || ch == 'b') {

				fmt.formatSpecifier = FormatSpecifier::B;
				fmt.formattedSize = 38;
			}
			else if (ch == 'P' || ch == 'p') {

				fmt.formatSpecifier = FormatSpecifier::P;
				fmt.formattedSize = 38;
			}
			else { 
				
				throw std::format_error{ "Invalid format specifier" };
			}

			fmt.uppercase = ch < 'a';
			return fmt;
		}

		constexpr auto FormattedSize() const noexcept -> std::size_t {

			return formattedSize;
		}

	private:

		template<typename CharT>
		friend class BasicGuidFormatString;

		friend struct GuidFormatter;

		constexpr BasicGuidFormatString() noexcept = default;

		FormatSpecifier formatSpecifier{};
		bool uppercase{};
		std::uint16_t formattedSize{};
	};

	struct GuidFormatter : GuidFormatBase {

		static consteval auto MaxFormattedSize() noexcept -> std::size_t {

			return 38;
		}

		static consteval auto FormattedSize() noexcept -> std::size_t {

			return 36;
		}

		template<typename CharT>
		static constexpr auto FormattedSize(BasicGuidFormatString<CharT> fmt) noexcept -> std::size_t {

			return fmt.FormattedSize();
		}

		template<std::same_as<char> CharT>
		static constexpr auto FormatTo(CharT* out, Guid value) noexcept -> CharT* {

			if constexpr (std::endian::native == std::endian::little) {

				value.Data1 = std::byteswap(value.Data1);
				value.Data2 = std::byteswap(value.Data2);
				value.Data3 = std::byteswap(value.Data3);
			}
			auto bytes = std::bit_cast<std::array<std::uint8_t, 16>>(value);

			out = Base16::EncodeLower({ &bytes[0], 4 }, out);
			*out++ = '-';
			out = Base16::EncodeLower({ &bytes[4], 2 }, out);
			*out++ = '-';
			out = Base16::EncodeLower({ &bytes[6], 2 }, out);
			*out++ = '-';
			out = Base16::EncodeLower({ &bytes[8], 2 }, out);
			*out++ = '-';
			out = Base16::EncodeLower({ &bytes[10], 6 }, out);
			return out;
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, Guid value) noexcept -> CharT* {

			auto buffer = TrivialArray<char, FormattedSize()>{};
			FormatTo<char>(buffer.data(), value);
			return std::ranges::copy(buffer, out).out;
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, Guid value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(), [value](CharT* data, std::size_t size) {

				FormatTo(data, value);
				return size;
			});
		}

		template<std::same_as<char> CharT>
		static constexpr auto FormatTo(CharT* out, BasicGuidFormatString<CharT> fmt, Guid value) noexcept -> CharT* {

			if constexpr (std::endian::native == std::endian::little) {

				value.Data1 = std::byteswap(value.Data1);
				value.Data2 = std::byteswap(value.Data2);
				value.Data3 = std::byteswap(value.Data3);
			}
			auto bytes = std::bit_cast<std::array<std::uint8_t, 16>>(value);

			using enum FormatSpecifier;
			switch (fmt.formatSpecifier) {
			case N: {

				if (fmt.uppercase)
					out = Base16::EncodeUpper(bytes, out);
				else
					out = Base16::EncodeLower(bytes, out);
				return out;
			}
			case D: break;
			case B: {

				out[0] = '{';
				out[37] = '}';
				++out;
			} break;
			case P: {

				out[0] = '(';
				out[37] = ')';
				++out;
			} break;
			default: std::unreachable();
			}
			
			if (fmt.uppercase) {

				out = Base16::EncodeUpper({ &bytes[0], 4 }, out);
				*out++ = '-';
				out = Base16::EncodeUpper({ &bytes[4], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeUpper({ &bytes[6], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeUpper({ &bytes[8], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeUpper({ &bytes[10], 6 }, out);
			}
			else {

				out = Base16::EncodeLower({ &bytes[0], 4 }, out);
				*out++ = '-';
				out = Base16::EncodeLower({ &bytes[4], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeLower({ &bytes[6], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeLower({ &bytes[8], 2 }, out);
				*out++ = '-';
				out = Base16::EncodeLower({ &bytes[10], 6 }, out);
			}
			return out + int{ fmt.formatSpecifier > D };
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, BasicGuidFormatString<CharT> fmt, Guid value) noexcept -> CharT* {

			auto buffer = TrivialArray<char, MaxFormattedSize()>{};
			auto end = FormatTo<char>(buffer.data(), GuidFormatString{ fmt }, value);
			return std::ranges::copy(buffer.data(), end, out).out;
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, BasicGuidFormatString<CharT> fmt, Guid value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(fmt), [fmt, value](CharT* data, std::size_t size) {

				FormatTo(data, fmt, value);
				return size;
			});
		}
	};

	inline constexpr auto Guid::Parse(std::string_view str) noexcept -> std::optional<Guid> {

		auto value = std::optional<Guid>{ std::in_place };
		if (!GuidParser::Parse(str, *value)) value.reset();
		return value;
	}

	inline constexpr auto Guid::Parse(std::string_view str, Guid& value) noexcept -> bool {

		return GuidParser::Parse(str, value);
	}

	inline constexpr auto Guid::Parse(std::wstring_view str) noexcept -> std::optional<Guid> {

		auto value = std::optional<Guid>{ std::in_place };
		if (!GuidParser::Parse(str, *value)) value.reset();
		return value;
	}

	inline constexpr auto Guid::Parse(std::wstring_view str, Guid& value) noexcept -> bool {

		return GuidParser::Parse(str, value);
	}

	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto Guid::Format() const -> std::basic_string<CharT> {

		auto str = std::basic_string<CharT>{};
		GuidFormatter::FormatTo(str, *this);
		return str;
	}

	inline constexpr auto Guid::Format(GuidFormatString fmt) const -> std::string {

		auto str = std::string{};
		GuidFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	inline constexpr auto Guid::Format(WGuidFormatString fmt) const -> std::wstring {

		auto str = std::wstring{};
		GuidFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto Guid::FormatTo(std::basic_string<CharT>& str) const -> void {

		return GuidFormatter::FormatTo(str, *this);
	}

	inline constexpr auto Guid::FormatTo(std::string& str, GuidFormatString fmt) const -> void {

		return GuidFormatter::FormatTo(str, fmt, *this);
	}

	inline constexpr auto Guid::FormatTo(std::wstring& str, WGuidFormatString fmt) const -> void {

		return GuidFormatter::FormatTo(str, fmt, *this);
	}

	inline consteval auto operator ""_Guid(char const* str, std::size_t size) -> Guid {

		auto guid = Guid{};
		if (!GuidParser::Parse<char>({ str, size }, guid))
			throw std::invalid_argument{ "Invalid guid string" };
		return guid;
	}
}

namespace std {

	template<>
	struct hash<::Citrine::Guid> {

		static constexpr auto operator()(::Citrine::Guid guid) noexcept -> size_t {

			using namespace ::Citrine;

			return FNV1a::AppendValue(FNV1a::OffsetBasis, guid);
		}
	};

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Guid, CharT> {

		constexpr auto parse(basic_format_parse_context<CharT>& ctx) -> auto {

			auto const begin = ctx.begin();
			auto it = begin;

			if (it != ctx.end() && *it != '}') {

				++it;
				fmt = FormatStringT::Parse({ begin, it });
			}
			return it;
		}

		auto format(::Citrine::Guid guid, auto& ctx) const -> auto {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, FormatterT::MaxFormattedSize()>{};
			auto end = FormatterT::FormatTo(buffer.data(), NarrowFormatStringT{ fmt }, guid);
			return std::copy(buffer.data(), end, ctx.out());
		}

	protected:

		using NarrowFormatStringT = ::Citrine::GuidFormatString;
		using FormatStringT = ::Citrine::BasicGuidFormatString<CharT>;
		using FormatterT = ::Citrine::GuidFormatter;

		FormatStringT fmt = basic_string_view<CharT>{};
	};
}

namespace glz {

	template<>
	struct from<JSON, ::Citrine::Guid>
	{
		template<auto Opts>
		static auto op(::Citrine::Guid& guid, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			if (!GuidParser::Parse(str, guid))
				ctx.error = error_code::parse_error;
		}
	};

	template<>
	struct to<JSON, ::Citrine::Guid>
	{
		template<auto Opts>
		static auto op(::Citrine::Guid guid, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, GuidFormatter::FormattedSize()>{};
			GuidFormatter::FormatTo(buffer.data(), guid);
			serialize<JSON>::op<Opts>(std::string_view{ buffer.data(), buffer.size() }, args...);
		}
	};
}
