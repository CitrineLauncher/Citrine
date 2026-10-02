#pragma once

#include "Core/Util/Event.h"

#include <winrt/Citrine.h>

namespace Citrine {

	class UpdateChecker {
	public:

		using StatusChangedEventHandler = EventHandler<winrt::Citrine::UpdateCheckStatus>;
		using UpdateAvailabilityChangedEventHandler = EventHandler<winrt::Citrine::UpdateAvailability>;

		static auto InitializeAsync() -> void;

		static auto Status() noexcept -> winrt::Citrine::UpdateCheckStatus;
		static auto StatusChanged(StatusChangedEventHandler handler) -> EventToken;
		static auto StatusChanged(EventToken&& token) -> void;

		static auto UpdateAvailability() noexcept -> winrt::Citrine::UpdateAvailability;
		static auto UpdateAvailabilityChanged(UpdateAvailabilityChangedEventHandler handler) -> EventToken;
		static auto UpdateAvailabilityChanged(EventToken&& token) -> void;

		static auto CheckForUpdatesAsync() -> void;
		static auto GetUpdateInfo() -> winrt::Citrine::UpdateInfo;

		static auto Shutdown() -> void;
	};
}
