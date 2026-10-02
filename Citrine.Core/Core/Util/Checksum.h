#pragma once

#include "Core/Util/StringLiteral.h"
#include "Core/Util/Concepts.h"
#include "Core/Codec/Base16.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Util/Hash.h"

#include <format>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine {

	template<typename Char>
	class BasicChecksumFormatString;

	using ChecksumFormatString = BasicChecksumFormatString<char>;
	using WChecksumFormatString = BasicChecksumFormatString<wchar_t>;

	template<StringLiteral Algorithm, std::size_t Size>
	struct BasicChecksum {

		using value_type = std::uint8_t;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = pointer;
		using const_iterator = const_pointer;

		static constexpr auto Parse(std::string_view str) noexcept -> std::optional<BasicChecksum>;
		static constexpr auto Parse(std::string_view str, BasicChecksum& value) noexcept -> bool;

		static constexpr auto Parse(std::wstring_view str) noexcept -> std::optional<BasicChecksum>;
		static constexpr auto Parse(std::wstring_view str, BasicChecksum& value) noexcept -> bool;

		static consteval auto AlgorithmName() -> std::string_view {

			return Algorithm;
		}

		constexpr auto operator[](size_type index) noexcept -> reference {

			return Data[index];
		}

		constexpr auto operator[](size_type index) const noexcept -> const_reference {

			return Data[index];
		}

		constexpr auto data() noexcept -> pointer {

			return Data;
		}

		constexpr auto data() const noexcept -> const_pointer {

			return Data;
		}

		constexpr auto begin() noexcept -> iterator {

			return Data;
		}

		constexpr auto begin() const noexcept -> const_iterator {

			return Data;
		}

		constexpr auto end() noexcept -> iterator {

			return Data + Size;
		}

		constexpr auto end() const noexcept -> const_iterator {

			return Data + Size;
		}

		static consteval auto size() noexcept -> size_type {

			return Size;
		}

		static consteval auto empty() noexcept -> bool {

			return Size == 0;
		}

		template<IsAnyOf<char, wchar_t> CharT = char>
		constexpr auto Format() const -> std::basic_string<CharT>;
		constexpr auto Format(ChecksumFormatString fmt) const -> std::string;
		constexpr auto Format(WChecksumFormatString fmt) const -> std::wstring;

		template<IsAnyOf<char, wchar_t> CharT>
		constexpr auto FormatTo(std::basic_string<CharT>& str) const -> void;
		constexpr auto FormatTo(std::string& str, ChecksumFormatString fmt) const -> void;
		constexpr auto FormatTo(std::wstring& str, WChecksumFormatString fmt) const -> void;

		template<IsAnyOf<char, wchar_t> CharT>
		explicit constexpr operator std::basic_string<CharT>() const {

			return Format<CharT>();
		}

		constexpr auto operator<=>(BasicChecksum const&) const noexcept = default;

		value_type Data[Size];
	};

	using SHA1Sum = BasicChecksum<"SHA-1", 20>;

	struct ChecksumParser {

		template<std::same_as<char> CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto Parse(std::basic_string_view<CharT> str, BasicChecksum<Algorithm, Size>& value) -> bool {

			constexpr auto encodedSize = Base16::EncodedSize(Size);
			if (str.size() != encodedSize)
				return false;

			return Base16::Decode(str, value.data());
		}

		template<std::same_as<wchar_t> CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto Parse(std::basic_string_view<CharT> str, BasicChecksum<Algorithm, Size>& value) -> bool {

			constexpr auto encodedSize = Base16::EncodedSize(Size);
			if (str.size() != encodedSize)
				return false;

			auto buffer = TrivialArray<char, encodedSize>{};
			for (auto out = buffer.data(); auto ch : str) {

				auto uch = static_cast<std::make_unsigned_t<CharT>>(ch);
				if (uch > 0x7f) uch = 0;
				*out++ = static_cast<char>(ch);
			}

			return Parse<char>({ buffer.data(), str.size() }, value);
		}
	};

	template<typename CharT>
	class BasicChecksumFormatString {
	public:

		template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicChecksumFormatString(T const& str) {

			*this = Parse(str);
		}

		template<typename CharT>
		explicit constexpr BasicChecksumFormatString(BasicChecksumFormatString<CharT> fmt)

			: includeAlgorithm(fmt.includeAlgorithm)
			, includeSeparator(fmt.includeSeperator)
			, includeDigest(fmt.includeDigest)
			, uppercase(fmt.uppercase)
		{}

		constexpr BasicChecksumFormatString(BasicChecksumFormatString const&) noexcept = default;
		constexpr auto operator=(BasicChecksumFormatString const&) noexcept -> BasicChecksumFormatString& = default;

		static constexpr auto Parse(std::basic_string_view<CharT> str) -> BasicChecksumFormatString {

			if (str.size() > 1)
				throw std::format_error{ "Too many format specifiers" };

			auto fmt = BasicChecksumFormatString{};
			auto ch = str.size() > 0
				? str.front()
				: 'x';

			if (ch == 'X' || ch == 'x') {

				fmt.includeDigest = true;
			}
			else if (ch == 'F' || ch == 'f') {

				fmt.includeAlgorithm = true;
				fmt.includeSeparator = true;
				fmt.includeDigest = true;
			}
			else if (ch == 'a') {

				fmt.includeAlgorithm = true;
			}
			else {

				throw std::format_error{ "Invalid format specifier" };
			}

			fmt.uppercase = ch < 'a';
			return fmt;
		}

		template<StringLiteral Algorithm, std::size_t Size>
		constexpr auto FormattedSize(BasicChecksum<Algorithm, Size> const&) const noexcept -> std::size_t {

			auto size = 0uz;
			if (includeAlgorithm)
				size += Algorithm.size();

			size += static_cast<std::size_t>(includeSeparator);
			if (includeDigest)
				size += Base16::EncodedSize(Size);

			return size;
		}

	private:

		template<typename CharT>
		friend class BasicChecksumFormatString;

		friend struct ChecksumFormatter;

		constexpr BasicChecksumFormatString() noexcept = default;

		bool includeAlgorithm{};
		bool includeSeparator{};
		bool includeDigest{};
		bool uppercase{};
	};

	struct ChecksumFormatter {

		template<StringLiteral Algorithm, std::size_t Size>
		static consteval auto MaxFormattedSize(BasicChecksum<Algorithm, Size> const&) noexcept -> std::size_t {

			return Algorithm.size() + 1 + Base16::EncodedSize(Size);
		}

		template<StringLiteral Algorithm, std::size_t Size>
		static consteval auto FormattedSize(BasicChecksum<Algorithm, Size> const&) noexcept -> std::size_t {

			return Base16::EncodedSize(Size);
		}

		template<typename CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormattedSize(BasicChecksumFormatString<CharT> fmt, BasicChecksum<Algorithm, Size> const& value) noexcept -> std::size_t {

			return fmt.FormattedSize(value);
		}

		template<std::same_as<char> CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(CharT* out, BasicChecksum<Algorithm, Size> const& value) noexcept -> CharT* {

			return Base16::EncodeLower(value, out);
		}

		template<typename CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(CharT* out, BasicChecksum<Algorithm, Size> const& value) noexcept -> CharT* {

			auto buffer = TrivialArray<char, FormattedSize(value)>{};
			FormatTo<char>(buffer.data(), value);
			return std::ranges::copy(buffer, out).out;
		}

		template<typename CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, BasicChecksum<Algorithm, Size> const& value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(value), [&value](CharT* data, std::size_t size) {

				FormatTo(data, value);
				return size;
			});
		}

		template<std::same_as<char> CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(CharT* out, BasicChecksumFormatString<CharT> fmt, BasicChecksum<Algorithm, Size> const& value) noexcept -> CharT* {

			if (fmt.includeAlgorithm)
				out = std::ranges::copy(Algorithm, out).out;

			if (fmt.includeSeparator)
				*out++ = ':';

			if (fmt.includeDigest) {

				if (fmt.uppercase)
					out = Base16::EncodeUpper(value, out);
				else
					out = Base16::EncodeLower(value, out);
			}

			return out;
		}

		template<typename CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(CharT* out, BasicChecksumFormatString<CharT> fmt, BasicChecksum<Algorithm, Size> const& value) noexcept -> CharT* {

			auto buffer = TrivialArray<char, MaxFormattedSize(value)>{};
			auto end = FormatTo<char>(buffer.data(), ChecksumFormatString{ fmt }, value);
			return std::ranges::copy(buffer.data(), end, out).out;
		}

		template<typename CharT, StringLiteral Algorithm, std::size_t Size>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, BasicChecksumFormatString<CharT> fmt, BasicChecksum<Algorithm, Size> const& value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(fmt, value), [fmt, &value](CharT* data, std::size_t size) {

				FormatTo(data, fmt, value);
				return size;
			});
		}
	};

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Parse(std::string_view str) noexcept -> std::optional<BasicChecksum> {

		auto value = std::optional<BasicChecksum>{ std::in_place };
		if (!ChecksumParser::Parse(str, *value)) value.reset();
		return value;
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Parse(std::string_view str, BasicChecksum& value) noexcept -> bool {

		return ChecksumParser::Parse(str, value);
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Parse(std::wstring_view str) noexcept -> std::optional<BasicChecksum> {

		auto value = std::optional<BasicChecksum>{ std::in_place };
		if (!ChecksumParser::Parse(str, *value)) value.reset();
		return value;
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Parse(std::wstring_view str, BasicChecksum& value) noexcept -> bool {

		return ChecksumParser::Parse(str, value);
	}

	template<StringLiteral Algorithm, std::size_t Size>
	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Format() const -> std::basic_string<CharT> {

		auto str = std::basic_string<CharT>{};
		ChecksumFormatter::FormatTo(str, *this);
		return str;
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Format(ChecksumFormatString fmt) const -> std::string {

		auto str = std::string{};
		ChecksumFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::Format(WChecksumFormatString fmt) const -> std::wstring {

		auto str = std::wstring{};
		ChecksumFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	template<StringLiteral Algorithm, std::size_t Size>
	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto BasicChecksum<Algorithm, Size>::FormatTo(std::basic_string<CharT>& str) const -> void {

		ChecksumFormatter::FormatTo(str, *this);
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::FormatTo(std::string& str, ChecksumFormatString fmt) const -> void {

		ChecksumFormatter::FormatTo(str, fmt, *this);
	}

	template<StringLiteral Algorithm, std::size_t Size>
	inline constexpr auto BasicChecksum<Algorithm, Size>::FormatTo(std::wstring& str, WChecksumFormatString fmt) const -> void {

		ChecksumFormatter::FormatTo(str, fmt, *this);
	}
}

namespace std {

	template<::Citrine::StringLiteral Algorithm, size_t Size>
	struct hash<::Citrine::BasicChecksum<Algorithm, Size>> {

		static constexpr auto operator()(::Citrine::BasicChecksum<Algorithm, Size> const& checksum) noexcept -> size_t {

			using namespace ::Citrine;

			return FNV1a::AppendValue(FNV1a::OffsetBasis, checksum);
		}
	};

	template<::Citrine::StringLiteral Algorithm, size_t Size, ::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::BasicChecksum<Algorithm, Size>, CharT> {

		constexpr auto parse(basic_format_parse_context<CharT>& ctx) -> auto {

			auto const begin = ctx.begin();
			auto it = begin;

			if (it != ctx.end() && *it != '}') {

				++it;
				fmt = FormatStringT::Parse({ begin, it });
			}
			return it;
		}

		auto format(::Citrine::BasicChecksum<Algorithm, Size> const& checksum, auto& ctx) const -> auto {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, FormatterT::MaxFormattedSize(checksum)>{};
			auto end = FormatterT::FormatTo(buffer.data(), NarrowFormatStringT{ fmt }, checksum);
			return std::copy(buffer.data(), end, ctx.out());
		}

	protected:

		using NarrowFormatStringT = ::Citrine::ChecksumFormatString;
		using FormatStringT = ::Citrine::BasicChecksumFormatString<CharT>;
		using FormatterT = ::Citrine::ChecksumFormatter;

		FormatStringT fmt = basic_string_view<CharT>{};
	};
}

namespace glz {

	template<::Citrine::StringLiteral Algorithm, size_t Size>
	struct from<JSON, ::Citrine::BasicChecksum<Algorithm, Size>>
	{
		template<auto Opts>
		static auto op(::Citrine::BasicChecksum<Algorithm, Size>& checksum, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			if (!ChecksumParser::Parse(str, checksum))
				ctx.error = error_code::parse_error;
		}
	};

	template<::Citrine::StringLiteral Algorithm, size_t Size>
	struct to<JSON, ::Citrine::BasicChecksum<Algorithm, Size>>
	{
		template<auto Opts>
		static auto op(::Citrine::BasicChecksum<Algorithm, Size> const& checksum, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, ChecksumFormatter::FormattedSize(checksum)>{};
			ChecksumFormatter::FormatTo(buffer.data(), checksum);
			serialize<JSON>::op<Opts>(std::string_view{ buffer.data(), buffer.size() }, args...);
		}
	};
}