#pragma once

#include "ByteProgressFormatter.g.h"

namespace winrt::Citrine::implementation
{
    struct ByteProgressFormatter
    {
        static auto Format(Citrine::ByteProgress const& progress, winrt::hstring const& ratioDelimiter) -> winrt::hstring;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ByteProgressFormatter : ByteProgressFormatterT<ByteProgressFormatter, implementation::ByteProgressFormatter>
    {
    };
}
