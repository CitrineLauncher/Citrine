#pragma once

#include "ParameterPair.g.h"

namespace winrt::Citrine::implementation
{
    struct ParameterPair : ParameterPairT<ParameterPair>
    {
        ParameterPair() = default;
        ParameterPair(winrt::Windows::Foundation::IInspectable first, winrt::Windows::Foundation::IInspectable second);

        auto First() const -> winrt::Windows::Foundation::IInspectable;
        auto First(winrt::Windows::Foundation::IInspectable const& value) -> void;

        auto Second() const -> winrt::Windows::Foundation::IInspectable;
        auto Second(winrt::Windows::Foundation::IInspectable const& value) -> void;

        winrt::Windows::Foundation::IInspectable first{ nullptr };
        winrt::Windows::Foundation::IInspectable second{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ParameterPair : ParameterPairT<ParameterPair, implementation::ParameterPair>
    {
    };
}
