#pragma once

#include "Core/Util/ParseInteger.h"
#include "Core/Util/FormatInteger.h"
#include "Core/Util/TrivialArray.h"

#include <string>

namespace Citrine {

	class ByteCountFormatBase {
	protected:

		static constexpr auto Scale = std::uint64_t{ 1024 };

		template<typename CharT>
		static constexpr auto Units = std::array<std::array<CharT, 4>, 9>{

			'b', 'y', 't', 'e',
			' ', 'K', 'i', 'B',
			' ', 'M', 'i', 'B',
			' ', 'G', 'i', 'B',
			' ', 'T', 'i', 'B',
			' ', 'P', 'i', 'B',
			' ', 'E', 'i', 'B',
			' ', 'Z', 'i', 'B',
			' ', 'Y', 'i', 'B'
		};
	};

	template<typename CharT>
	class BasicByteCountFormatString {
	public:

		template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicByteCountFormatString(T const& str) {

			*this = Parse(str);
		}

		template<typename CharT>
		explicit constexpr BasicByteCountFormatString(BasicByteCountFormatString<CharT> fmt)

			: precision(fmt.precision)
		{}

		constexpr BasicByteCountFormatString(BasicByteCountFormatString const&) noexcept = default;
		constexpr auto operator=(BasicByteCountFormatString const&) noexcept -> BasicByteCountFormatString& = default;

		static constexpr auto Parse(std::basic_string_view<CharT> str) -> BasicByteCountFormatString {

			auto fmt = BasicByteCountFormatString{};
			if (str.empty())
				return fmt;

			if (str.size() != 3 || str[0] != '.')
				throw std::format_error{ "Invalid specifier" };

			auto precision = DigitFromChar(str[1]);
			if (precision > 2 || str[2] != 'f')
				throw std::format_error{ "Invalid precision" };

			fmt.precision = precision;
			return fmt;
		}

	private:

		template<typename CharT>
		friend class BasicByteCountFormatString;

		friend struct ByteCountFormatter;

		constexpr BasicByteCountFormatString() noexcept = default;

		int precision{ -1 };
	};

	using ByteCountFormatString = BasicByteCountFormatString<char>;
	using WByteCountFormatString = BasicByteCountFormatString<wchar_t>;

	struct ByteCountFormatter : ByteCountFormatBase {

		static consteval auto MaxFormattedSize() noexcept -> std::size_t {

			return 13; // count(7) + space(1) + unit(5)
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, std::uint64_t value) noexcept -> CharT* {

			return FormatTo<CharT>(out, std::basic_string_view<CharT>{}, value);
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, std::uint64_t value) noexcept -> void {

			auto buffer = TrivialArray<CharT, MaxFormattedSize()>{};
			return output.assign(buffer.data(), FormatTo<CharT>(output, std::basic_string_view<CharT>{}, value));
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, BasicByteCountFormatString<CharT> fmt, std::uint64_t value) noexcept -> CharT* {

			auto unit = std::uint64_t{};
			auto fraction = std::uint64_t{};

			if (value >= Scale) {

				auto fp = static_cast<double>(value);
				do {

					fp /= Scale;
					++unit;
				} while (fp >= Scale);

				value = static_cast<std::uint64_t>(fp);
				fraction = static_cast<std::uint64_t>((fp - value) * 100);
			}

			if (value >= 10) {

				if (value >= 100) {

					if (value >= 1000)
						out = std::copy_n("10", 2, out);
					else
						*out++ = value / 100 + '0';
					value %= 100;
				}
				out = std::copy_n(&Radix100Table<CharT>[value * 2], 2, out);
			}
			else { *out++ = value + '0'; }

			if (unit == 0) {

				*out++ = ' ';
				out = std::ranges::copy(Units<CharT>[0], out).out;
				*out = 's';
				return out + static_cast<std::ptrdiff_t>(value != 1);
			}

			if (fmt.precision > 0) {

				*out++ = '.';
				std::copy_n(&Radix100Table<CharT>[fraction * 2], 2, out);
				out += fmt.precision;
			}
			else if (fmt.precision == -1 && fraction > 0) {

				*out++ = '.';
				std::copy_n(&Radix100Table<CharT>[fraction * 2], 2, out++);
				out += static_cast<std::ptrdiff_t>(fraction % 10 != 0);
			}

			return std::ranges::copy(Units<CharT>[unit], out).out;
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, BasicByteCountFormatString<CharT> fmt, std::uint64_t value) noexcept -> void {

			auto buffer = TrivialArray<CharT, MaxFormattedSize()>{};
			output.assign(buffer.data(), FormatTo(buffer.data(), fmt, value));
		}
	};

	template<typename CharT = char>
	class FormatByteCount {
	public:

		explicit constexpr FormatByteCount(std::uint64_t value) noexcept

			: strEnd(ByteCountFormatter::FormatTo(buffer, value))
		{}

		explicit constexpr FormatByteCount(BasicByteCountFormatString<CharT> fmt, std::uint64_t value) noexcept

			: strEnd(ByteCountFormatter::FormatTo(buffer, fmt, value))
		{}

		FormatByteCount(FormatByteCount const&) = delete;
		auto operator=(FormatByteCount const&) = delete;

		constexpr auto begin() const noexcept -> CharT const* {

			return buffer;
		}

		constexpr auto end() const noexcept -> CharT const* {

			return strEnd;
		}

		constexpr auto operator[](std::size_t index) const noexcept -> CharT const& {

			return buffer[index];
		}

		constexpr auto data() const noexcept -> CharT const* {

			return buffer;
		}

		constexpr auto size() const noexcept -> std::size_t {

			return strEnd - buffer;
		}

		constexpr operator std::basic_string_view<CharT>() const noexcept {

			return { data(), size() };
		}

	private:

		static constexpr auto BufferSize = ByteCountFormatter::MaxFormattedSize();

		CharT buffer[BufferSize];
		CharT const* strEnd;
	};

	template<typename T>
	FormatByteCount(T, std::uint64_t) -> FormatByteCount<std::remove_cvref_t<decltype(std::declval<T>()[0])>>;
}