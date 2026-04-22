#pragma once

#include "NumberBoxEx.g.h"

namespace winrt::Citrine::implementation
{
    struct NumberBoxEx : NumberBoxExT<NumberBoxEx>
    {
        NumberBoxEx();

        auto ClearButtonVisibility() -> winrt::Microsoft::UI::Xaml::Visibility;
        auto ClearButtonVisibility(winrt::Microsoft::UI::Xaml::Visibility value) -> void;
        static auto ClearButtonVisibilityProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto SuffixText() -> winrt::hstring;
        auto SuffixText(winrt::hstring const& value) -> void;
        static auto SuffixTextProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto EnsureProperties() -> void;

    private:

        static winrt::Microsoft::UI::Xaml::DependencyProperty clearButtonVisibilityProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty suffixTextProperty;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct NumberBoxEx : NumberBoxExT<NumberBoxEx, implementation::NumberBoxEx>
    {
    };
}
