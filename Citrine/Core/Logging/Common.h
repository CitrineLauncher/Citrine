#pragma once

namespace Citrine {

    enum struct LogLevel {

        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    struct LogSourceLocation {

        static consteval auto Current(char const* fileName = __builtin_FILE(), int line = __builtin_LINE()) -> LogSourceLocation {

            return { fileName, line };
        }

        char const* FileName{};
        int Line{};
    };
}