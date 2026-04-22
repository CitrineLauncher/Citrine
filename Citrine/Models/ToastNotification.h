#pragma once

#include "ToastNotification.g.h"

namespace winrt::Citrine::implementation
{
    struct ToastNotification : ToastNotificationT<ToastNotification>
    {
        ToastNotification(Citrine::NotificationSeverity severity, winrt::hstring title) noexcept;
        ToastNotification(Citrine::NotificationSeverity severity, winrt::hstring title, winrt::hstring message) noexcept;

        auto Severity() const noexcept -> Citrine::NotificationSeverity;
        auto Title() const noexcept -> winrt::hstring;
        auto Message() const noexcept -> winrt::hstring;

        Citrine::NotificationSeverity severity{};
        winrt::hstring title;
        winrt::hstring message;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ToastNotification : ToastNotificationT<ToastNotification, implementation::ToastNotification>
    {
    };
}
