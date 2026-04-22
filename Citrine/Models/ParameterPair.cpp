#include "pch.h"
#include "ParameterPair.h"
#if __has_include("ParameterPair.g.cpp")
#include "ParameterPair.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation;
}

namespace winrt::Citrine::implementation
{
    ParameterPair::ParameterPair(winrt::IInspectable first, winrt::IInspectable second)

        : first(std::move(first))
        , second(std::move(second))
    {}

    auto ParameterPair::First() const -> winrt::IInspectable {

        return first;
    }

    auto ParameterPair::First(winrt::IInspectable const& value) -> void {

        first = value;
    }

    auto ParameterPair::Second() const -> winrt::IInspectable {

        return second;
    }

    auto ParameterPair::Second(winrt::IInspectable const& value) -> void {

        second = value;
    }
}
