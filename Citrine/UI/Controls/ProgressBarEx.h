#pragma once

#include "ProgressBarEx.g.h"

namespace winrt::Citrine::implementation
{
    struct ProgressBarEx : ProgressBarExT<ProgressBarEx>
    {
        ProgressBarEx();

        auto IsIndeterminate() const -> bool;
        auto IsIndeterminate(bool value) -> void;
        static auto IsIndeterminateProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto Status() const -> Citrine::ProgressBarStatus;
        auto Status(Citrine::ProgressBarStatus value) -> void;
        static auto StatusProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto TemplateSettings() const -> Citrine::ProgressBarExTemplateSettings;
        static auto TemplateSettingsProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto OnApplyTemplate() -> void;

    private:

        auto EnsureProperties() -> void;
        auto OnPropertyChanged(this winrt::Microsoft::UI::Xaml::DependencyObject const& sender, winrt::Windows::Foundation::IInspectable const&) -> void;

        auto OnSizeChanged() -> void;

        auto UpdateProgressBarVisualState() -> void;
        auto UpdateProgressBarIndicatorWidth(bool useRepositionAnimation = false) -> void;
        auto UpdateWidthBasedTemplateSettings() -> void;

        winrt::Microsoft::UI::Xaml::Controls::Grid layoutRoot{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Rectangle determinateProgressBarIndicator{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Rectangle indeterminateProgressBarIndicator{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Rectangle indeterminateProgressBarIndicator2{ nullptr };

        static winrt::Microsoft::UI::Xaml::DependencyProperty isIndeterminateProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty statusProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty templateSettingsProperty;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ProgressBarEx : ProgressBarExT<ProgressBarEx, implementation::ProgressBarEx>
    {
    };
}
