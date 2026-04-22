#pragma once

#include "Convert.g.h"

namespace winrt::Citrine::implementation
{
    struct Convert
    {
        static auto BoolToInverseBool(bool value) noexcept -> bool;

        static auto BoolToVisibility(bool value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;
        static auto BoolToInverseVisibility(bool value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;

        static auto NullToBool(winrt::Windows::Foundation::IInspectable const& value) noexcept -> bool;
        static auto NullToInverseBool(winrt::Windows::Foundation::IInspectable const& value) noexcept -> bool;

        static auto NullToVisibility(winrt::Windows::Foundation::IInspectable const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;
        static auto NullToInverseVisibility(winrt::Windows::Foundation::IInspectable const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;

        static auto StringToBool(winrt::hstring const& value) noexcept -> bool;
        static auto StringToInverseBool(winrt::hstring const& value) noexcept -> bool;

        static auto StringToVisibility(winrt::hstring const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;
        static auto StringToInverseVisibility(winrt::hstring const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct Convert : ConvertT<Convert, implementation::Convert>
    {
    };
}
