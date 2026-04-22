#pragma once

#include "Common.h"
#include "LogFormatter.h"
#include "LogMessage.h"

#include <filesystem>

namespace Citrine {
	
	template<typename CharT, typename... Args>
	struct BasicLogFormatStringWithLocation : BasicLogFormatString<CharT, Args...> {

        template<std::convertible_to<std::basic_string_view<CharT>> T>
		consteval BasicLogFormatStringWithLocation(T const& fmt, LogSourceLocation source = LogSourceLocation::Current())

			: BasicLogFormatString<CharT, Args...>(fmt)
			, Source(source)
		{}

		LogSourceLocation Source;
	};

	template<typename... Args>
	using LogFormatStringWithLocation = BasicLogFormatStringWithLocation<char, std::type_identity_t<Args>...>;

	template<typename... Args>
	using WLogFormatStringWithLocation = BasicLogFormatStringWithLocation<wchar_t, std::type_identity_t<Args>...>;

	class Logger {
	public:

#ifdef _DEBUG
		static constexpr auto ActiveLogLevel = LogLevel::Debug;
#else
		static constexpr auto ActiveLogLevel = LogLevel::Info;
#endif

        static auto Initialize(std::filesystem::path path) -> void;

        template<typename... Args>
        static auto Trace(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Trace>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Trace(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Trace>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        template<typename... Args>
        static auto Debug(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Debug>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Debug(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Debug>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        template<typename... Args>
        static auto Info(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Info>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Info(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Info>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        template<typename... Args>
        static auto Warn(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Warn>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Warn(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Warn>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        template<typename... Args>
        static auto Error(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Error>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Error(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Error>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        template<typename... Args>
        static auto Critical(LogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Critical>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, char>(args)...);
        }

        template<typename... Args>
        static auto Critical(WLogFormatStringWithLocation<Args...> fmt, Args&&... args) -> void {

            LogInternal<LogLevel::Critical>(fmt.Source, fmt.Fmt, ForwardAsLogArgument<Args, wchar_t>(args)...);
        }

        static auto Flush() -> void;
        static auto Shutdown() -> void;

	private:

        template<typename CharT>
        static thread_local inline auto messageBuffer = std::basic_string<CharT>{};

        template<LogLevel Level, typename CharT, typename... Args>
        static auto LogInternal(LogSourceLocation source, std::basic_format_string<CharT, Args...> fmt, Args&&... args) -> void {

            if constexpr (Level >= ActiveLogLevel) {

                auto& buffer = messageBuffer<CharT>;
                buffer.clear();
                std::vformat_to(std::back_inserter(buffer), fmt.get(), std::make_format_args<BasicLogFormatContext<CharT>>(args...));
                PostLogMessage({ LogMessageType::Log, Level, DateTime::Now(), source, buffer });
            }
        }

        static auto PostLogMessage(LogMessage&& message) -> void;
	};
}
