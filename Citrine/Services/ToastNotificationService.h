#pragma once

#include "Models/ToastNotification.h"
#include "Core/Util/Event.h"

namespace Citrine {

	class ToastNotificationService {
	public:

		using NotificationHandler = EventHandler<winrt::Citrine::ToastNotification const&>;

		static auto Subscribe(NotificationHandler handler) -> EventToken;
		static auto Unsubscribe(EventToken&& token) -> void;

		static auto SendNotification(winrt::Citrine::NotificationSeverity severity, winrt::hstring title, winrt::hstring message) -> void;
	};
}