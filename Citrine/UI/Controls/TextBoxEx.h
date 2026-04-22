#pragma once

#include "TextBoxEx.g.h"

namespace winrt::Citrine::implementation
{
    struct TextBoxEx : TextBoxExT<TextBoxEx>
    {
        TextBoxEx();

        auto ClearButtonVisibility() -> winrt::Microsoft::UI::Xaml::Visibility;
        auto ClearButtonVisibility(winrt::Microsoft::UI::Xaml::Visibility value) -> void;
        static auto ClearButtonVisibilityProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto PrefixText() -> winrt::hstring;
        auto PrefixText(winrt::hstring const& value) -> void;
        static auto PrefixTextProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto SuffixText() -> winrt::hstring;
        auto SuffixText(winrt::hstring const& value) -> void;
        static auto SuffixTextProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

    private:

        auto EnsureProperties() -> void;

        static winrt::Microsoft::UI::Xaml::DependencyProperty clearButtonVisibilityProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty prefixTextProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty suffixTextProperty;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct TextBoxEx : TextBoxExT<TextBoxEx, implementation::TextBoxEx>
    {
    };
}
