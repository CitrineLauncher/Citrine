#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Util/TrivialArray.h"

#include <chrono>
#include <string>
#include <array>
#include <utility>
#include <optional>
#include <format>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine {

	using namespace std::chrono_literals;

	template<typename CharT>
	class BasicDateTimeFormatString;

	using DateTimeFormatString = BasicDateTimeFormatString<char>;
	using WDateTimeFormatString = BasicDateTimeFormatString<wchar_t>;

	struct DateTime {

		using TimePointT = std::chrono::system_clock::time_point;
		using DurationT = std::chrono::system_clock::duration;
		using DateT = std::chrono::year_month_day;
		using TimeT = std::chrono::hh_mm_ss<std::chrono::nanoseconds>;

		static DateTime const MinValue;
		static DateTime const MaxValue;

		constexpr DateTime() noexcept = default;

		constexpr DateTime(TimePointT timePoint) noexcept

			: TimePoint(timePoint)
		{}

		constexpr DateTime(DateT date) noexcept

			: TimePoint(std::chrono::sys_days{ date })
		{}

		constexpr DateTime(DateT date, auto const& duration) noexcept

			: DateTime(date)
		{
			TimePoint += duration;
		}

		constexpr DateTime(DateTime const&) noexcept = default;
		auto operator=(DateTime const&) noexcept -> DateTime& = default;

		static constexpr auto Parse(std::string_view str) noexcept -> std::optional<DateTime>;
		static constexpr auto Parse(std::string_view str, DateTime& value) noexcept -> bool;

		static constexpr auto Parse(std::wstring_view str) noexcept -> std::optional<DateTime>;
		static constexpr auto Parse(std::wstring_view str, DateTime& value) noexcept -> bool;

		static auto Now() noexcept -> DateTime {

			return TimePointT::clock::now();
		}

		constexpr auto Date() noexcept -> DateT {

			return DateT{ std::chrono::floor<std::chrono::days>(TimePoint) };
		}

		constexpr auto Time() noexcept -> TimeT {

			return TimeT{ TimePoint - std::chrono::floor<std::chrono::days>(TimePoint) };
		}

		friend constexpr auto operator+(DateTime dateTime, auto const& duration) noexcept -> DateTime {

			return dateTime.TimePoint + duration;
		}

		friend constexpr auto operator+(auto const& duration, DateTime dateTime) noexcept -> DateTime {

			return duration + dateTime.TimePoint;
		}

		friend constexpr auto operator-(DateTime dateTime, auto const& duration) noexcept -> DateTime {

			return dateTime.TimePoint - duration;
		}

		friend constexpr auto operator-(auto const& duration, DateTime dateTime) noexcept -> DateTime {

			return duration - dateTime.TimePoint;
		}

		constexpr auto operator+=(auto const& duration) noexcept -> DateTime& {

			TimePoint += duration;
			return *this;
		}

		constexpr auto operator-=(auto const& duration) noexcept -> DateTime& {

			TimePoint -= duration;
			return *this;
		}

		template<IsAnyOf<char, wchar_t> CharT = char>
		constexpr auto Format() const -> std::basic_string<CharT>;
		constexpr auto Format(DateTimeFormatString const& fmt) const -> std::string;
		constexpr auto Format(WDateTimeFormatString const& fmt) const -> std::wstring;

		template<IsAnyOf<char, wchar_t> CharT>
		constexpr auto FormatTo(std::basic_string<CharT>& str) const -> void;
		constexpr auto FormatTo(std::string& str, DateTimeFormatString const& fmt) const -> void;
		constexpr auto FormatTo(std::wstring& str, WDateTimeFormatString const& fmt) const -> void;

		template<IsAnyOf<char, wchar_t> CharT>
		explicit constexpr operator std::basic_string<CharT>() const {

			return Format<CharT>();
		}

		constexpr auto operator<=>(DateTime const&) const noexcept -> std::strong_ordering = default;

		TimePointT TimePoint;
	};

	struct DateTimeHelper {

		static constexpr auto ToDuration(auto const& duration) noexcept -> DateTime::DurationT {

			return std::chrono::duration_cast<DateTime::DurationT>(duration);
		}

		static constexpr auto Clamp(DateTime value) noexcept -> DateTime {

			if (value < DateTime::MinValue)
				return DateTime::MinValue;
			if (value > DateTime::MaxValue)
				return DateTime::MaxValue;
			return value;
		}

		static constexpr auto Pow10 = std::array<std::uint32_t, 10>{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
	};
	
	inline constexpr DateTime DateTime::MinValue = { 0y / 1 / 1 };
	inline constexpr DateTime DateTime::MaxValue = { 9999y / 12 / 31, 23h + 59min + 59s + DateTimeHelper::ToDuration(999'999'999ns) };

	struct DateTimeParser {

		template<typename CharT>
		static constexpr auto Parse(std::basic_string_view<CharT> str, DateTime& value) noexcept -> bool {

			auto it = str.data();
			auto const end = it + str.size();

			auto parseSeg = [&](std::size_t count, std::uint32_t& value) -> bool {

				if (end - it < static_cast<std::ptrdiff_t>(count)) return false;
				for (auto last = it + count; it < last; ++it) {

					auto digit = DigitFromChar(*it);
					if (digit >= 10) return false;
					value = value * 10 + digit;
				}
				return true;
			};

			auto assignDate = [&](std::uint32_t year, std::uint32_t month, std::uint32_t day) -> bool {

				auto const date = std::chrono::year{ static_cast<int>(year) } / month / day;
				if (!date.ok()) return false;
				value = date;
				return true;
			};

			auto year = std::uint32_t{};
			if (!parseSeg(4, year)) return false;
			if (it == end) return assignDate(year, 1, 1);
			if (*it++ != '-') return false;

			auto month = std::uint32_t{};
			if (!parseSeg(2, month)) return false;
			if (it == end) return assignDate(year, month, 1);
			if (*it++ != '-') return false;

			auto day = std::uint32_t{};
			if (!parseSeg(2, day) || !assignDate(year, month, day)) return false;
			if (it == end) return true;
			if (*it++ != 'T') return false;

			auto hours = std::uint32_t{};
			if (!parseSeg(2, hours) || hours > 23) return false;
			if (it == end) return false;
			if (*it++ != ':') return false;

			auto minutes = std::uint32_t{};
			if (!parseSeg(2, minutes) || minutes > 59) return false;
			if (it == end) return false;

			auto seconds = std::uint32_t{};
			if (*it == ':') {

				if (++it; !parseSeg(2, seconds) || seconds > 59) return false;
				if (it == end) return false;
			}

			value += std::chrono::hours{ hours };
			value += std::chrono::minutes{ minutes };
			value += std::chrono::seconds{ seconds };

			if (*it == '.' && end - it >= 3) {

				auto const first = ++it;
				auto const last = end - 1;

				auto subseconds = std::uint32_t{};
				do {

					auto digit = DigitFromChar(*it++);
					if (digit >= 10) break;
					subseconds = subseconds * 10 + digit;
				} while (it < last);

				auto digits = it - first;
				if (digits < 1 || digits > 9) return false;
				value += DateTimeHelper::ToDuration(std::chrono::nanoseconds{ subseconds * DateTimeHelper::Pow10[9 - digits] });
			}

			switch (auto designator = *it++) {
			case '+': [[fallthrough]];
			case '-': {

				if (end - it != 5) return false;
				auto hoursOffset = std::uint32_t{};
				if (!parseSeg(2, hoursOffset) || hoursOffset > 23) return false;

				if (*it++ != ':') return false;
				auto minutesOffset = std::uint32_t{};
				if (!parseSeg(2, minutesOffset) || minutesOffset > 59) return false;

				auto offset = std::chrono::hours{ hoursOffset } + std::chrono::minutes{ minutesOffset };
				if (designator == '+')
					value -= offset;
				else
					value += offset;

			} [[fallthrough]];
			case 'Z': return it == end;
			}
			return false;
		}
	};

	class DateTimeFormatBase {
	protected:

		enum struct SegmentType : std::uint8_t {

			Year,
			Month,
			Day,
			Hours,
			Minutes,
			Seconds,
			UtcOffset,
			Terminator
		};

		struct Segment {

			SegmentType Type{};
			std::uint8_t Precision{};
			std::uint16_t Position{};
		};
	};

	template<typename CharT>
	class BasicDateTimeFormatString : DateTimeFormatBase {
	public:

		template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicDateTimeFormatString(T const& str) {

			*this = Parse(str);
		}

		constexpr BasicDateTimeFormatString(BasicDateTimeFormatString const&) noexcept = default;
		constexpr auto operator=(BasicDateTimeFormatString const&) noexcept -> BasicDateTimeFormatString& = default;

		static constexpr auto Parse(std::basic_string_view<CharT> str) -> BasicDateTimeFormatString {

			auto fmt = BasicDateTimeFormatString{};
			fmt.str = str.data();
			if (str.empty()) {

				fmt.segments[0] = { SegmentType::Terminator };
				fmt.formattedSize = 28;
				return fmt;
			}

			auto it = str.data();
			auto const end = it + str.size();

			auto lastSegment = fmt.segments;
			auto insertSegment = [&, bitset = int{}](SegmentType segment, std::uint8_t precision, std::uint16_t offset) mutable {

				auto pos = 1 << std::to_underlying(segment);
				if ((bitset & pos) != 0)
					throw std::format_error{ "Segment specified more than once" };

				bitset |= pos;
				*lastSegment++ = { segment, precision, offset };
			};

			auto currentSeparator = it;
			while (it < end) {

				auto offset = static_cast<std::uint16_t>(it - currentSeparator);
				switch (*it++) {
				case '%': {

					switch (*it++) {
					case 'D': {

						insertSegment(SegmentType::Day, 0, offset);
						fmt.formattedSize += 2;
					} break;
					case 'M': {

						insertSegment(SegmentType::Month, 0, offset);
						fmt.formattedSize += 2;
					} break;
					case 'Y': {

						insertSegment(SegmentType::Year, 0, offset);
						fmt.formattedSize += 4;
					} break;
					case 'h': {

						insertSegment(SegmentType::Hours, 0, offset);
						fmt.formattedSize += 2;
					} break;
					case 'm': {

						insertSegment(SegmentType::Minutes, 0, offset);
						fmt.formattedSize += 2;
					} break;
					case 's': {

						auto precision = std::uint8_t{};
						if (end - it >= 3 && *it == '.') {

							precision = DigitFromChar(*++it);
							if (++it; precision > 9 || *it++ != 'f')
								throw std::format_error{ "Invalid precision" };
						}
						insertSegment(SegmentType::Seconds, precision, offset);
						fmt.formattedSize += precision > 0 ? 3 + precision : 2;
					} break;
					case 'z': {

						insertSegment(SegmentType::UtcOffset, 0, offset);
						++fmt.formattedSize;
					} break;
					default: {

						throw std::format_error{ "Invalid specifier" };
					}
					}
					currentSeparator = it;
				} break;
				case '.': {

					throw std::format_error{ "Invalid delimiter" };
				}
				case '\\': {

					if (end - it < 1)
						throw std::format_error{ "" };
					++it;
					++fmt.formattedSize;
				} break;
				default: {

					++fmt.formattedSize;
				} break;
				}
			}

			if (lastSegment == fmt.segments) throw std::format_error{ "Specify at least one segment" };
			insertSegment(SegmentType::Terminator, 0, static_cast<std::uint16_t>(it - currentSeparator));
			return fmt;
		}

		constexpr auto FormattedSize() const noexcept -> std::size_t {

			return formattedSize;
		}

		constexpr auto IsEmpty() const noexcept -> bool {

			return segments[0].Type == SegmentType::Terminator;
		}

	private:

		friend struct DateTimeFormatter;

		constexpr BasicDateTimeFormatString() noexcept = default;

		CharT const* str{};
		Segment segments[8];
		std::size_t formattedSize{};
	};

	struct DateTimeFormatter : DateTimeFormatBase {

		static consteval auto FormattedSize() noexcept -> std::size_t {

			return 28;
		}

		template<typename CharT>
		static constexpr auto FormattedSize(BasicDateTimeFormatString<CharT> const& fmt) noexcept -> std::size_t {

			return fmt.FormattedSize();
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, DateTime value) noexcept -> CharT* {

			auto timepoint = DateTimeHelper::Clamp(value).TimePoint;
			auto days = std::chrono::floor<std::chrono::days>(timepoint);
			auto date = DateTime::DateT{ days };
			auto time = DateTime::TimeT{ std::chrono::floor<std::chrono::nanoseconds>(timepoint - days) };

			auto formatSeg = [&](auto value, std::size_t digits) {

				while (digits > 0) {

					auto const pow10 = DateTimeHelper::Pow10[--digits];
					auto const digit = value / pow10;
					*out++ = '0' + static_cast<char>(digit);
					value -= digit * pow10;
				}
			};

			formatSeg(static_cast<int>(date.year()), 4);
			*out++ = '-';
			formatSeg(static_cast<unsigned int>(date.month()), 2);
			*out++ = '-';
			formatSeg(static_cast<unsigned int>(date.day()), 2);
			*out++ = 'T';
			formatSeg(time.hours().count(), 2);
			*out++ = ':';
			formatSeg(time.minutes().count(), 2);
			*out++ = ':';
			formatSeg(time.seconds().count(), 2);
			*out++ = '.';
			formatSeg(time.subseconds().count() / 100, 7);
			*out++ = 'Z';

			return out;
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, DateTime value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(), [value](CharT* data, std::size_t size) {

				FormatTo(data, value);
				return size;
			});
		}

		template<typename CharT>
		static constexpr auto FormatTo(CharT* out, BasicDateTimeFormatString<CharT> const& fmt, DateTime value) noexcept -> CharT* {

			if (fmt.IsEmpty())
				return FormatTo(out, value);

			auto timepoint = DateTimeHelper::Clamp(value).TimePoint;
			auto days = std::chrono::floor<std::chrono::days>(timepoint);
			auto date = DateTime::DateT{ days };
			auto time = DateTime::TimeT{ std::chrono::floor<std::chrono::nanoseconds>(timepoint - days) };

			auto formatSeg = [&](auto value, std::size_t digits) {

				while (digits > 0) {

					auto const pow10 = DateTimeHelper::Pow10[--digits];
					auto const digit = value / pow10;
					*out++ = '0' + static_cast<char>(digit);
					value -= digit * pow10;
				}
			};

			auto it = fmt.str;
			for (auto seg : fmt.segments) {

				for (auto end = it + seg.Position; it < end;) {

					auto ch = *it++;
					if (ch == '\\') ch = *it++;
					*out++ = ch;
				}

				using enum SegmentType;
				switch (seg.Type) {
				case Year:
					formatSeg(static_cast<int>(date.year()), 4);
					break;
				case Month:
					formatSeg(static_cast<unsigned int>(date.month()), 2);
					break;
				case Day:
					formatSeg(static_cast<unsigned int>(date.day()), 2);
					break;
				case Hours:
					formatSeg(time.hours().count(), 2);
					break;
				case Minutes:
					formatSeg(time.minutes().count(), 2);
					break;
				case Seconds: {

					formatSeg(time.seconds().count(), 2);
					if (auto precision = seg.Precision; precision > 0) {

						*out++ = '.';
						formatSeg(time.subseconds().count() / DateTimeHelper::Pow10[9 - precision], precision);
						it += 3; // Skip precision specifier
					}
				} break;
				case UtcOffset:
					*out++ = 'Z';
					break;
				case Terminator:
					return out;
				default:
					std::unreachable();
				}
				it += 2; // Skip segment specifier
			}
			std::unreachable();
		}

		template<typename CharT>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, BasicDateTimeFormatString<CharT> const& fmt, DateTime value) noexcept -> void {

			output.resize_and_overwrite(FormattedSize(fmt), [&fmt, value](CharT* data, std::size_t size) {

				FormatTo(data, fmt, value);
				return size;
			});
		}
	};

	inline constexpr auto DateTime::Parse(std::string_view str) noexcept -> std::optional<DateTime> {

		auto value = std::optional<DateTime>{ std::in_place };
		if (!DateTimeParser::Parse(str, *value)) value.reset();
		return value;
	}

	inline constexpr auto DateTime::Parse(std::string_view str, DateTime& value) noexcept -> bool {

		return DateTimeParser::Parse(str, value);
	}

	inline constexpr auto DateTime::Parse(std::wstring_view str) noexcept -> std::optional<DateTime> {

		auto value = std::optional<DateTime>{ std::in_place };
		if (!DateTimeParser::Parse(str, *value)) value.reset();
		return value;
	}

	inline constexpr auto DateTime::Parse(std::wstring_view str, DateTime& value) noexcept -> bool {

		return DateTimeParser::Parse(str, value);
	}

	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto DateTime::Format() const -> std::basic_string<CharT> {

		auto str = std::basic_string<CharT>{};
		DateTimeFormatter::FormatTo(str, *this);
		return str;
	}

	inline constexpr auto DateTime::Format(DateTimeFormatString const& fmt) const -> std::string {

		auto str = std::string{};
		DateTimeFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	inline constexpr auto DateTime::Format(WDateTimeFormatString const& fmt) const -> std::wstring {

		auto str = std::wstring{};
		DateTimeFormatter::FormatTo(str, fmt, *this);
		return str;
	}

	template<IsAnyOf<char, wchar_t> CharT>
	inline constexpr auto DateTime::FormatTo(std::basic_string<CharT>& str) const -> void {

		DateTimeFormatter::FormatTo(str, *this);
	}

	inline constexpr auto DateTime::FormatTo(std::string& str, DateTimeFormatString const& fmt) const -> void {

		DateTimeFormatter::FormatTo(str, fmt, *this);
	}

	inline constexpr auto DateTime::FormatTo(std::wstring& str, WDateTimeFormatString const& fmt) const -> void {

		DateTimeFormatter::FormatTo(str, fmt, *this);
	}
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::DateTime, CharT> {

		constexpr auto parse(basic_format_parse_context<CharT>& ctx) -> auto {

			auto const begin = ctx.begin();
			auto it = begin;

			for (auto const end = ctx.end(); it != end; ++it) {

				if (*it == '{' || *it == '}') {

					fmt = FormatStringT::Parse({ begin, it });
					break;
				}
			}
			return it;
		}

		auto format(::Citrine::DateTime dateTime, auto& ctx) const -> auto {

			FormatterT::FormatTo(buffer, fmt, dateTime);
			return std::ranges::copy(buffer, ctx.out()).out;
		}

	protected:

		using FormatStringT = ::Citrine::BasicDateTimeFormatString<CharT>;
		using FormatterT = ::Citrine::DateTimeFormatter;

		static thread_local inline auto buffer = basic_string<CharT>{};
		FormatStringT fmt = basic_string_view<CharT>{};
	};
}

namespace Citrine::Glaze {

	template<typename T>
		requires std::same_as<std::remove_cvref_t<T>, DateTime>
	struct UnixTimeT {

		T& Val;
	};

	template<auto M>
	inline constexpr auto UnixTime = [](auto& val) static { return UnixTimeT{ val.*M }; };
}

namespace glz {

	template<>
	struct from<JSON, ::Citrine::DateTime>
	{
		template<auto Opts>
		static auto op(::Citrine::DateTime& dateTime, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			if (!DateTimeParser::Parse(str, dateTime))
				ctx.error = error_code::parse_error;
		}
	};

	template<>
	struct to<JSON, ::Citrine::DateTime>
	{
		template<auto Opts>
		static auto op(::Citrine::DateTime dateTime, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, DateTimeFormatter::FormattedSize()>{};
			DateTimeFormatter::FormatTo(buffer.data(), dateTime);
			serialize<JSON>::op<Opts>(std::string_view{ buffer.data(), buffer.size() }, args...);
		}
	};

	using ::Citrine::Glaze::UnixTime;

	template<typename T>
	struct from<JSON, ::Citrine::Glaze::UnixTimeT<T>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) -> void {

			using Seconds = std::chrono::seconds;
			using TimePoint = T::TimePointT;

			auto rep = Seconds::rep{};
			parse<JSON>::op<Opts>(rep, args...);
			value.Val = TimePoint{ Seconds{ rep } };
		}
	};

	template<typename T>
	struct to<JSON, ::Citrine::Glaze::UnixTimeT<T>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) noexcept -> void {

			using Seconds = std::chrono::seconds;

			auto dur = value.Val.TimePoint.time_since_epoch();
			auto rep = std::chrono::floor<Seconds>(dur).count();
			serialize<JSON>::op<Opts>(rep, args...);
		}
	};
}