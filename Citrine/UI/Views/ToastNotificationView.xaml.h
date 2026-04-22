#pragma once

#include "ToastNotificationView.g.h"

namespace winrt::Citrine::implementation
{
    struct ToastNotificationView : ToastNotificationViewT<ToastNotificationView>
    {
        ToastNotificationView();

        auto Notification() const -> Citrine::ToastNotification;
        auto Notification(Citrine::ToastNotification const& value) -> void;
        static auto NotificationProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto CloseCommand() const -> winrt::Microsoft::UI::Xaml::Input::ICommand;
        auto CloseCommand(winrt::Microsoft::UI::Xaml::Input::ICommand const& value) -> void;
        static auto CloseCommandProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty;
        
        auto OnPointerEntered(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) -> void;
        auto OnPointerExited(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) -> void;

        auto CloseButton_GotFocus(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto CloseButton_LostFocus(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto CloseButton_PointerCaptureLost(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
        auto CloseButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;

    private:

        auto EnsureProperties() -> void;

        auto ExecuteCloseCommand() -> void;

        auto OnLoaded() -> void;
        auto OnUnloaded() -> void;

        static winrt::Microsoft::UI::Xaml::DependencyProperty notificationProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty closeCommandProperty;

        bool pointerOver{};
        Microsoft::UI::Xaml::Media::Animation::Storyboard storyboard{ nullptr };
        Microsoft::UI::Xaml::Media::Animation::Storyboard::Completed_revoker storyboardCompletedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ToastNotificationView : ToastNotificationViewT<ToastNotificationView, implementation::ToastNotificationView>
    {
    };
}
