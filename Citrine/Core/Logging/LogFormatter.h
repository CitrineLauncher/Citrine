#pragma once

#include "Core/Util/Concepts.h"

#include <format>
#include <type_traits>
#include <filesystem>

namespace Citrine {

	template<typename T, typename CharT>
	struct LogFormatter;

	template<typename T, typename CharT>
	concept LogFormattable = requires{ LogFormatter<std::remove_cvref_t<T>, CharT>{}; };

	template<typename T>
	struct LogFormattableArgument {

		T const& Value;
	};

	template<typename Arg, typename CharT>
	using LogArgumentT = std::conditional_t<LogFormattable<Arg, CharT>, LogFormattableArgument<std::remove_cvref_t<Arg>>, Arg>;

	template<typename Arg, typename CharT>
	constexpr auto ForwardAsLogArgument(auto&& arg) noexcept -> decltype(auto) {

		if constexpr (LogFormattable<Arg, CharT>)
			return LogFormattableArgument{ arg };
		else
			return std::forward<Arg>(arg);
	}

	template<IsAnyOf<char, wchar_t> CharT>
	using BasicLogFormatContext = std::conditional_t<std::same_as<CharT, wchar_t>,
		std::wformat_context,
		std::format_context
	>;

	using LogFormatContext = BasicLogFormatContext<char>;
	using WLogFormatContext = BasicLogFormatContext<wchar_t>;

	template<IsAnyOf<char, wchar_t> CharT, typename... Args>
	struct BasicLogFormatString {

		template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicLogFormatString(T const& fmt) : Fmt(fmt) {}

		std::basic_format_string<CharT, LogArgumentT<Args, CharT>...> Fmt;
	};

	template<typename... Args>
	using LogFormatString = BasicLogFormatString<char, std::type_identity_t<Args>...>;

	template<typename... Args>
	using WLogFormatString = BasicLogFormatString<wchar_t, std::type_identity_t<Args>...>;

	struct LogFormatterBase {

		template<typename... Args>
		static auto FormatTo(LogFormatContext::iterator out, LogFormatString<Args...> fmt, Args&&... args) -> auto {

			return std::format_to(out, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
		}

		template<typename... Args>
		static auto FormatTo(WLogFormatContext::iterator out, WLogFormatString<Args...> fmt, Args&&... args) -> auto {

			return std::format_to(out, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
		}
	};

	template<IsAnyOf<char, wchar_t> CharT>
	struct LogFormatter<std::filesystem::path, CharT> {

		static constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) -> auto {

			return ctx.begin();
		}

		auto format(std::filesystem::path const& value, BasicLogFormatContext<CharT>& ctx) const -> decltype(ctx.out());
	};
}

namespace std {

	template<typename T, typename CharT>
	struct formatter<::Citrine::LogFormattableArgument<T>, CharT> : ::Citrine::LogFormatter<T, CharT> {

		auto format(::Citrine::LogFormattableArgument<T> arg, ::Citrine::BasicLogFormatContext<CharT>& ctx) const -> auto {

			return ::Citrine::LogFormatter<T, CharT>::format(arg.Value, ctx);
		}
	};
}