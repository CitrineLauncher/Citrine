#include "pch.h"
#include "ToastNotificationService.h"

using namespace Citrine;
using namespace winrt::Citrine;

namespace {

	Event<ToastNotificationService::NotificationHandler> notificationHandlers;
}

namespace Citrine {

	auto ToastNotificationService::Subscribe(NotificationHandler handler) -> EventToken {

		return notificationHandlers.Add(std::move(handler));
	}

	auto ToastNotificationService::Unsubscribe(EventToken&& token) -> void {

		notificationHandlers.Remove(std::move(token));
	}

	auto ToastNotificationService::SendNotification(NotificationSeverity severity, winrt::hstring title, winrt::hstring message) -> void {
	
		auto notification = winrt::make<implementation::ToastNotification>(severity, std::move(title), std::move(message));
		notificationHandlers(notification);
	}
}