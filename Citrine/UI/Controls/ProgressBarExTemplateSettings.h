#pragma once

#include "ProgressBarExTemplateSettings.g.h"

namespace winrt::Citrine::implementation
{
    struct ProgressBarExTemplateSettings : ProgressBarExTemplateSettingsT<ProgressBarExTemplateSettings>
    {
        ProgressBarExTemplateSettings();

        auto ContainerAnimationStartPosition() const -> double;
        auto ContainerAnimationStartPosition(double value) -> void;
        static auto ContainerAnimationStartPositionProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto ContainerAnimationEndPosition() const -> double;
        auto ContainerAnimationEndPosition(double value) -> void;
        static auto ContainerAnimationEndPositionProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto Container2AnimationStartPosition() const -> double;
        auto Container2AnimationStartPosition(double value) -> void;
        static auto Container2AnimationStartPositionProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto Container2AnimationEndPosition() const -> double;
        auto Container2AnimationEndPosition(double value) -> void;
        static auto Container2AnimationEndPositionProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto ContainerAnimationMidPosition() const -> double;
        auto ContainerAnimationMidPosition(double value) -> void;
        static auto ContainerAnimationMidPositionProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto IndicatorLengthDelta() const -> double;
        auto IndicatorLengthDelta(double value)-> void;
        static auto IndicatorLengthDeltaProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto ClipRect() const -> winrt::Microsoft::UI::Xaml::Media::RectangleGeometry;
        auto ClipRect(winrt::Microsoft::UI::Xaml::Media::RectangleGeometry const& value) -> void;
        static auto ClipRectProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty;

    private:

        auto EnsureProperties() -> void;

        static winrt::Microsoft::UI::Xaml::DependencyProperty containerAnimationStartPositionProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty containerAnimationEndPositionProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty container2AnimationStartPositionProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty container2AnimationEndPositionProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty containerAnimationMidPositionProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty indicatorLengthDeltaProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty clipRectProperty;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ProgressBarExTemplateSettings : ProgressBarExTemplateSettingsT<ProgressBarExTemplateSettings, implementation::ProgressBarExTemplateSettings>
    {
    };
}
