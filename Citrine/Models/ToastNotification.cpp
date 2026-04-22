#include "pch.h"
#include "ToastNotification.h"
#if __has_include("ToastNotification.g.cpp")
#include "ToastNotification.g.cpp"
#endif

namespace winrt::Citrine::implementation
{
    ToastNotification::ToastNotification(Citrine::NotificationSeverity severity, winrt::hstring title) noexcept

        : severity(severity)
        , title(std::move(title))
    {}

    ToastNotification::ToastNotification(Citrine::NotificationSeverity severity, winrt::hstring title, winrt::hstring message) noexcept

        : severity(severity)
        , title(std::move(title))
        , message(std::move(message))
    {}

    auto ToastNotification::Severity() const noexcept -> Citrine::NotificationSeverity {

        return severity;
    }

    auto ToastNotification::Title() const noexcept -> winrt::hstring {

        return title;
    }

    auto ToastNotification::Message() const noexcept -> winrt::hstring {

        return message;
    }
}
