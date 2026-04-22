#include "pch.h"
#include "ProgressBarEx.h"
#if __has_include("ProgressBarEx.g.cpp")
#include "ProgressBarEx.g.cpp"
#endif

#include "ProgressBarExTemplateSettings.h"

namespace winrt {

    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Microsoft::UI::Xaml::Controls::Primitives;
    using namespace Microsoft::UI::Xaml::Shapes;
}

namespace {

    auto LayoutRound(double value, double scaleFactor) -> double {

        return std::round(value * scaleFactor) / scaleFactor;
    }
}

namespace winrt::Citrine::implementation
{
	ProgressBarEx::ProgressBarEx() {

        DefaultStyleKey(winrt::box_value(winrt::xaml_typename<class_type>()));

        EnsureProperties();
        RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), &ProgressBarEx::OnPropertyChanged);
        RegisterPropertyChangedCallback(winrt::RangeBase::MinimumProperty(), &ProgressBarEx::OnPropertyChanged);
        RegisterPropertyChangedCallback(winrt::RangeBase::MaximumProperty(), &ProgressBarEx::OnPropertyChanged);
        RegisterPropertyChangedCallback(winrt::Control::PaddingProperty(), &ProgressBarEx::OnPropertyChanged);
        RegisterPropertyChangedCallback(winrt::UIElement::VisibilityProperty(), &ProgressBarEx::OnPropertyChanged);

        SizeChanged([this](auto const&...) { OnSizeChanged(); });
        SetValue(templateSettingsProperty, winrt::make<implementation::ProgressBarExTemplateSettings>());
	}

    auto ProgressBarEx::IsIndeterminate() const -> bool {

        return winrt::unbox_value<bool>(GetValue(isIndeterminateProperty));
    }

    auto ProgressBarEx::IsIndeterminate(bool value) -> void {

        SetValue(isIndeterminateProperty, winrt::box_value(value));
    }
    
    auto ProgressBarEx::IsIndeterminateProperty() -> winrt::DependencyProperty {

        return isIndeterminateProperty;
    }

    auto ProgressBarEx::Status() const -> Citrine::ProgressBarStatus {

        return winrt::unbox_value<Citrine::ProgressBarStatus>(GetValue(statusProperty));
    }

    auto ProgressBarEx::Status(Citrine::ProgressBarStatus value) -> void {

        SetValue(statusProperty, winrt::box_value(value));
    }

    auto ProgressBarEx::StatusProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty {

        return statusProperty;
    }

    auto ProgressBarEx::TemplateSettings() const -> Citrine::ProgressBarExTemplateSettings {

        return GetValue(templateSettingsProperty).as<Citrine::ProgressBarExTemplateSettings>();
    }

    auto ProgressBarEx::TemplateSettingsProperty() -> winrt::Microsoft::UI::Xaml::DependencyProperty {

        return templateSettingsProperty;
    }

    auto ProgressBarEx::OnApplyTemplate() -> void {

        layoutRoot = GetTemplateChild(L"LayoutRoot").try_as<winrt::Grid>();
        determinateProgressBarIndicator = GetTemplateChild(L"DeterminateProgressBarIndicator").try_as<winrt::Rectangle>();
        indeterminateProgressBarIndicator = GetTemplateChild(L"IndeterminateProgressBarIndicator").try_as<winrt::Rectangle>();
        indeterminateProgressBarIndicator2 = GetTemplateChild(L"IndeterminateProgressBarIndicator2").try_as<winrt::Rectangle>();

        UpdateProgressBarVisualState();
    }

    auto ProgressBarEx::EnsureProperties() -> void {

        if (!isIndeterminateProperty) {

            isIndeterminateProperty = winrt::DependencyProperty::Register(
                L"IsIndeterminate",
                winrt::xaml_typename<bool>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(false), &ProgressBarEx::OnPropertyChanged }
            );
        }

        if (!statusProperty) {

            statusProperty = winrt::DependencyProperty::Register(
                L"Status",
                winrt::xaml_typename<Citrine::ProgressBarStatus>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ winrt::box_value(ProgressBarStatus::Active), &ProgressBarEx::OnPropertyChanged }
            );
        }

        if (!templateSettingsProperty) {

            templateSettingsProperty = winrt::DependencyProperty::Register(
                L"TemplateSettings",
                winrt::xaml_typename<Citrine::ProgressBarExTemplateSettings>(),
                winrt::xaml_typename<class_type>(),
                nullptr
            );
        }
    }

    auto ProgressBarEx::OnPropertyChanged(this winrt::DependencyObject const& sender, winrt::Windows::Foundation::IInspectable const&) -> void {

        auto self = sender.try_as<ProgressBarEx>();
        if (!self)
            return;

        self->UpdateProgressBarIndicatorWidth(true);
        self->UpdateProgressBarVisualState();
    }

    auto ProgressBarEx::OnSizeChanged() -> void {

        UpdateProgressBarIndicatorWidth();
        UpdateWidthBasedTemplateSettings();
    }

    auto ProgressBarEx::UpdateProgressBarVisualState() -> void {

        using enum ProgressBarStatus;
        auto status = Status();

        auto visualState = std::wstring_view{};
        if (IsIndeterminate() && Visibility() == winrt::Visibility::Visible) {

            switch (status) {
            case Paused:    visualState = L"IndeterminatePaused";   break;
            case Error:     visualState = L"IndeterminateError";    break;
            default:        visualState = L"Indeterminate";         break;
            }
            UpdateWidthBasedTemplateSettings();
        }
        else {

            switch (status) {
            case Paused:    visualState = L"Paused";        break;
            case Error:     visualState = L"Error";         break;
            default:        visualState = L"Determinate";   break;
            }
        }
        winrt::VisualStateManager::GoToState(*this, visualState, true);
    }

    auto ProgressBarEx::UpdateProgressBarIndicatorWidth(bool useRepositionAnimation) -> void {

        auto templateSettings = winrt::get_self<implementation::ProgressBarExTemplateSettings>(TemplateSettings());

        auto& progressBar = layoutRoot;
        if (!progressBar)
            return;

        auto progressBarWidth = progressBar.ActualWidth();
        auto maximum = Maximum();
        auto minimum = Minimum();
        auto padding = Padding();
        auto borderThickness = BorderThickness();

        auto scaleFactor = 1.0;
        if (auto xamlRoot = progressBar.XamlRoot())
            scaleFactor = xamlRoot.RasterizationScale();

        auto roundedBorderWidth = progressBar.UseLayoutRounding()
            ? LayoutRound(borderThickness.Left, scaleFactor) + LayoutRound(borderThickness.Right, scaleFactor)
            : borderThickness.Left + borderThickness.Right;

        auto paddingAndBorderWidth = padding.Left + padding.Right + roundedBorderWidth;
        auto maxIndicatorWidth = std::max(progressBarWidth - paddingAndBorderWidth, 0.0);

        using enum ProgressBarStatus;
        auto status = Status();

        if (IsIndeterminate()) {

            if (determinateProgressBarIndicator)
                determinateProgressBarIndicator.Width(0);

            if (indeterminateProgressBarIndicator)
                indeterminateProgressBarIndicator.Width(maxIndicatorWidth * 0.4);

            if (indeterminateProgressBarIndicator2)
                indeterminateProgressBarIndicator2.Width((status == Paused || status == Error) ? maxIndicatorWidth : maxIndicatorWidth * 0.6);

            return;
        }
        
        if (!determinateProgressBarIndicator)
            return;

        if (std::abs(maximum - minimum) > std::numeric_limits<double>::epsilon())  {

            if (status == Active && useRepositionAnimation)
                winrt::VisualStateManager::GoToState(*this, L"Updating", true);

            auto prevIndicatorWidth = determinateProgressBarIndicator.ActualWidth();
            auto increment = maxIndicatorWidth / (maximum - minimum);
            auto indicatorWidth = increment * (Value() - minimum);
            auto widthDelta = indicatorWidth - prevIndicatorWidth;

            templateSettings->IndicatorLengthDelta(-widthDelta);
            determinateProgressBarIndicator.Width(indicatorWidth);

            if (status == Active && useRepositionAnimation)
                winrt::VisualStateManager::GoToState(*this, L"Determinate", true);
        }
        else {

            determinateProgressBarIndicator.Width(0);
        }
    }

    auto ProgressBarEx::UpdateWidthBasedTemplateSettings() -> void {

        auto templateSettings = winrt::get_self<implementation::ProgressBarExTemplateSettings>(TemplateSettings());

        auto [width, height] = [&progressBar = layoutRoot] -> std::pair<double, double> {

            if (progressBar) {

                auto width = static_cast<float>(progressBar.ActualWidth());
                auto height = static_cast<float>(progressBar.ActualHeight());
                return { width, height };
            }
            return { 0.0f, 0.0f };
        }();

        auto indeterminateProgressBarIndicatorWidth = width * 0.4;
        auto indeterminateProgressBarIndicatorWidth2 = width * 0.6;

        templateSettings->ContainerAnimationStartPosition(indeterminateProgressBarIndicatorWidth * -1.0);
        templateSettings->ContainerAnimationEndPosition(indeterminateProgressBarIndicatorWidth * 3.0);

        templateSettings->Container2AnimationStartPosition(indeterminateProgressBarIndicatorWidth2 * -1.5);
        templateSettings->Container2AnimationEndPosition(indeterminateProgressBarIndicatorWidth2 * 1.66);

        templateSettings->ContainerAnimationMidPosition(0);
    }

    winrt::Microsoft::UI::Xaml::DependencyProperty ProgressBarEx::isIndeterminateProperty{ nullptr };
    winrt::Microsoft::UI::Xaml::DependencyProperty ProgressBarEx::statusProperty{ nullptr };
    winrt::Microsoft::UI::Xaml::DependencyProperty ProgressBarEx::templateSettingsProperty{ nullptr };
}
