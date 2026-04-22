#include "pch.h"
#include "ProgressBarExTemplateSettings.h"
#if __has_include("ProgressBarExTemplateSettings.g.cpp")
#include "ProgressBarExTemplateSettings.g.cpp"
#endif

namespace winrt {

	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Media;
}

namespace winrt::Citrine::implementation
{
	ProgressBarExTemplateSettings::ProgressBarExTemplateSettings() {

        EnsureProperties();
	}

    auto ProgressBarExTemplateSettings::ContainerAnimationStartPosition() const -> double {

        return winrt::unbox_value<double>(GetValue(containerAnimationStartPositionProperty));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationStartPosition(double value) -> void {

        SetValue(containerAnimationStartPositionProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationStartPositionProperty() -> winrt::DependencyProperty {

        return containerAnimationStartPositionProperty;
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationEndPosition() const -> double {

        return winrt::unbox_value<double>(GetValue(containerAnimationEndPositionProperty));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationEndPosition(double value) -> void {

        SetValue(containerAnimationEndPositionProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationEndPositionProperty() -> winrt::DependencyProperty {

        return containerAnimationEndPositionProperty;
    }

    auto ProgressBarExTemplateSettings::Container2AnimationStartPosition() const -> double {

        return winrt::unbox_value<double>(GetValue(container2AnimationStartPositionProperty));
    }

    auto ProgressBarExTemplateSettings::Container2AnimationStartPosition(double value) -> void {

        SetValue(container2AnimationStartPositionProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::Container2AnimationStartPositionProperty() -> winrt::DependencyProperty {

        return container2AnimationStartPositionProperty;
    }

    auto ProgressBarExTemplateSettings::Container2AnimationEndPosition() const -> double {

        return winrt::unbox_value<double>(GetValue(container2AnimationEndPositionProperty));
    }

    auto ProgressBarExTemplateSettings::Container2AnimationEndPosition(double value) -> void {

        SetValue(container2AnimationEndPositionProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::Container2AnimationEndPositionProperty() -> winrt::DependencyProperty {

        return container2AnimationEndPositionProperty;
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationMidPosition() const -> double {

        return winrt::unbox_value<double>(GetValue(containerAnimationMidPositionProperty));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationMidPosition(double value) -> void {

        SetValue(containerAnimationMidPositionProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::ContainerAnimationMidPositionProperty() -> winrt::DependencyProperty {

        return containerAnimationMidPositionProperty;
    }

    auto ProgressBarExTemplateSettings::IndicatorLengthDelta() const -> double {

        return winrt::unbox_value<double>(GetValue(indicatorLengthDeltaProperty));
    }

    auto ProgressBarExTemplateSettings::IndicatorLengthDelta(double value) -> void {

        SetValue(indicatorLengthDeltaProperty, winrt::box_value(value));
    }

    auto ProgressBarExTemplateSettings::IndicatorLengthDeltaProperty() -> winrt::DependencyProperty {

        return indicatorLengthDeltaProperty;
    }

    auto ProgressBarExTemplateSettings::ClipRect() const -> winrt::RectangleGeometry {

        return GetValue(clipRectProperty).try_as<winrt::RectangleGeometry>();
    }

    auto ProgressBarExTemplateSettings::ClipRect(winrt::RectangleGeometry const& value) -> void {

        SetValue(clipRectProperty, value);
    }

    auto ProgressBarExTemplateSettings::ClipRectProperty() -> winrt::DependencyProperty {

        return clipRectProperty;
    }

	auto ProgressBarExTemplateSettings::EnsureProperties() -> void {

        if (!containerAnimationStartPositionProperty) {

            containerAnimationStartPositionProperty = winrt::DependencyProperty::Register(
                L"ContainerAnimationStartPosition",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!containerAnimationEndPositionProperty) {

            containerAnimationEndPositionProperty = winrt::DependencyProperty::Register(
                L"ContainerAnimationEndPosition",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!container2AnimationStartPositionProperty) {

            container2AnimationStartPositionProperty = winrt::DependencyProperty::Register(
                L"Container2AnimationStartPosition",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!container2AnimationEndPositionProperty) {

            container2AnimationEndPositionProperty = winrt::DependencyProperty::Register(
                L"Container2AnimationEndPosition",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!containerAnimationMidPositionProperty) {

            containerAnimationMidPositionProperty = winrt::DependencyProperty::Register(
                L"ContainerAnimationMidPosition",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!indicatorLengthDeltaProperty) {

            indicatorLengthDeltaProperty = winrt::DependencyProperty::Register(
                L"IndicatorLengthDelta",
                winrt::xaml_typename<double>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(0.0) }
            );
        }

        if (!clipRectProperty) {

            clipRectProperty = winrt::DependencyProperty::Register(
                L"ClipRect",
                winrt::xaml_typename<winrt::RectangleGeometry>(),
                winrt::xaml_typename<class_type>(),
                nullptr
            );
        }
	}

	winrt::DependencyProperty ProgressBarExTemplateSettings::containerAnimationStartPositionProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::containerAnimationEndPositionProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::container2AnimationStartPositionProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::container2AnimationEndPositionProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::containerAnimationMidPositionProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::indicatorLengthDeltaProperty{ nullptr };
	winrt::DependencyProperty ProgressBarExTemplateSettings::clipRectProperty{ nullptr };
}
