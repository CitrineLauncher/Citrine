#include "pch.h"
#include "UpdateChecker.h"

#include "Citrine.h"

#include "Models/UpdateInfo.h"

#include "Core/Coroutine/Task.h"
#include "Core/Coroutine/FireAndForget.h"
#include "Core/Logging/Logger.h"
#include "Core/Util/Scope.h"
#include "Core/Util/SemanticVersion.h"
#include "Core/Util/DateTime.h"
#include "ApplicationData.h"
#include "Services/HttpService.h"
#include "Services/NetworkInfoService.h"

#include <winrt/Windows.System.Threading.h>

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::System::Threading;
	using namespace Microsoft::UI::Dispatching;
}

namespace {

	using winrt::Citrine::UpdateCheckStatus;
	using winrt::Citrine::UpdateAvailability;
	using winrt::Citrine::UpdateInfo;
	using UpdateInfoImpl = winrt::Citrine::implementation::UpdateInfo;

	auto GetReleaseUrl(ReleaseInfo const& release) -> winrt::Uri {

		return winrt::Uri{ L"https://github.com/CitrineLauncher/Citrine/releases/tag/", winrt::to_hstring(release.Tag) };
	}

	auto SelectRelease(ReleaseInfoCollection const& releases, ReleaseChannel releaseChannel) -> ReleaseInfo const* {

		using enum ReleaseChannel;

		constexpr auto currentVersion = SemanticVersion{ CITRINE_VERSION_MAJOR, CITRINE_VERSION_MINOR, CITRINE_VERSION_PATCH };

		auto selectedRelease = static_cast<ReleaseInfo const*>(nullptr);
		for (auto const& release : releases) {

			if (release.Version <= currentVersion)
				continue;

			if (release.Channel != releaseChannel && release.Channel != Stable)
				continue;

			if (!selectedRelease || release.Version > selectedRelease->Version)
				selectedRelease = &release;
		}

		return selectedRelease;
	}

	class UpdateCheckerInternal {
	public:

		using StatusChangedEventHandler = UpdateChecker::StatusChangedEventHandler;
		using UpdateAvailabilityChangedEventHandler = UpdateChecker::UpdateAvailabilityChangedEventHandler;

		static constexpr auto RefreshInterval = 4h;

		UpdateCheckerInternal() = default;

		auto InitializeAsync() -> void {

			Logger::Info("Initializing UpdateChecker");

			dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();

			auto& appSettings = ApplicationData::LocalSettings();
			auto automaticUpdateChecks = appSettings.AutomaticUpdateChecks();

			automaticUpdateChecksChangedToken = appSettings.AutomaticUpdateChecksChanged({ this, &UpdateCheckerInternal::OnAutomaticUpdateChecksChanged });
			updateChannelChangedToken = appSettings.UpdateChannelChanged({ this, &UpdateCheckerInternal::OnUpdateChannelChanged });

			networkConnectivityChangedToken = NetworkInfoService::NetworkConnectivityChanged({ this, &UpdateCheckerInternal::OnNetworkConnectivityChanged });

			initialized = true;
			if (automaticUpdateChecks)
				CheckForUpdatesAsync();
		}

		auto Status() const noexcept -> winrt::Citrine::UpdateCheckStatus {

			return status;
		}

		auto StatusChanged(StatusChangedEventHandler&& handler) -> EventToken {
		
			return statusChangedEvent.Add(std::move(handler));
		}

		auto StatusChanged(EventToken&& token) -> void {

			statusChangedEvent.Remove(std::move(token));
		}

		auto UpdateAvailability() noexcept -> winrt::Citrine::UpdateAvailability {

			return updateAvailability;
		}

		auto UpdateAvailabilityChanged(UpdateAvailabilityChangedEventHandler handler) -> EventToken {

			return updateAvailabilityChangedEvent.Add(std::move(handler));
		}

		auto UpdateAvailabilityChanged(EventToken&& token) -> void {

			updateAvailabilityChangedEvent.Remove(std::move(token));
		}

		auto CheckForUpdatesAsync() -> FireAndForget {

			using enum UpdateCheckStatus;

			if (!initialized)
				co_return;

			if (std::exchange(status, Checking) == Checking)
				co_return;

			if (refreshTimer)
				std::exchange(refreshTimer, nullptr).Cancel();

			Logger::Info("Checking for updates");

			auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, "https://raw.githubusercontent.com/CitrineLauncher/Citrine/main/Latest.json");
			
			auto& appSettings = ApplicationData::LocalSettings();
			auto automaticUpdateChecks = appSettings.AutomaticUpdateChecks();
			auto updateChannel = appSettings.UpdateChannel();
			
			if (automaticUpdateChecks) {

				refreshTimer = winrt::ThreadPoolTimer::CreateTimer({ this, &UpdateCheckerInternal::OnRefreshTimerElapsed }, RefreshInterval);
			}
			lastRefreshTime = DateTime::Now();

			if (!responseMessage) {

				Logger::Error("Checking for updates failed: network error");
				status = NetworkError;
				co_return;
			}

			if (!responseMessage->IsSuccessful()) {

				Logger::Error("Checking for updates failed: api error");
				status = ApiError;
				co_return;
			}
			
			auto latestReleases = ReleaseInfoCollection{};
			
			constexpr auto opts = glz::opts{ .null_terminated = false };
			if (auto ec = glz::read<opts>(latestReleases, responseMessage->Content); ec) {

				Logger::Error("Checking for updates failed: response error");
				status = ResponseError;
				co_return;
			}
			releases = std::move(latestReleases);

			auto selectedRelease = SelectRelease(releases, updateChannel);
			OnRefresh(selectedRelease);

			Logger::Info("Checking for updates completed");
			status = Idle;
		}

		auto GetUpdateInfo() const noexcept -> UpdateInfo {

			return updateInfo;
		}

	private:

		auto OnAutomaticUpdateChecksChanged(bool automaticUpdateChecks) -> void {

			using enum UpdateAvailability;

			if (automaticUpdateChecks) {

				if (refreshTimer)
					return;

				auto delta = DateTime::Now().TimePoint - lastRefreshTime.TimePoint;

				if (updateAvailability == Unknown || delta >= RefreshInterval) {

					CheckForUpdatesAsync();
					return;
				}

				refreshTimer = winrt::ThreadPoolTimer::CreateTimer({ this, &UpdateCheckerInternal::OnRefreshTimerElapsed }, delta);
			}
			else {

				if (refreshTimer)
					std::exchange(refreshTimer, nullptr).Cancel();
			}
		}

		auto OnUpdateChannelChanged(ReleaseChannel updateChannel) -> void {

			using enum UpdateAvailability;

			if (updateAvailability == Unknown)
				return;

			auto selectedRelease = SelectRelease(releases, updateChannel);
			OnRefresh(selectedRelease);
		}

		auto OnNetworkConnectivityChanged(NetworkConnectivity connectivity) -> FireAndForget {

			using enum UpdateAvailability;
			using enum NetworkConnectivity;

			if (connectivity != InternetAccess)
				co_return;

			try {

				co_await wil::resume_foreground(dispatcherQueue, winrt::DispatcherQueuePriority::Low);
			}
			catch (winrt::hresult_error const&) {

				co_return;
			}

			auto& appSettings = ApplicationData::LocalSettings();
			auto automaticUpdateChecks = appSettings.AutomaticUpdateChecks();

			if (automaticUpdateChecks && updateAvailability == Unknown) {

				CheckForUpdatesAsync();
			}
		}

		auto OnRefreshTimerElapsed(winrt::ThreadPoolTimer timer) -> FireAndForget {

			try {

				co_await wil::resume_foreground(dispatcherQueue, winrt::DispatcherQueuePriority::Low);
			}
			catch (winrt::hresult_error const&) {

				co_return;
			}

			if (timer != refreshTimer)
				co_return;

			CheckForUpdatesAsync();
		}

		auto OnRefresh(ReleaseInfo const* selectedRelease) -> void {

			using enum UpdateAvailability;

			auto oldTag = updateInfo
				? std::string_view{ winrt::get_self<UpdateInfoImpl>(updateInfo)->ReleaseInfo().Tag }
				: "";

			auto newTag = selectedRelease
				? std::string_view{ selectedRelease->Tag }
				: "";

			if (newTag == oldTag) {

				if (updateAvailability == Unknown)
					updateAvailability = NoUpdates;
				return;
			}

			if (selectedRelease) {

				Logger::Info("New update available: {}", *selectedRelease);
				updateInfo = winrt::make<UpdateInfoImpl>(*selectedRelease, GetReleaseUrl(*selectedRelease));
				updateAvailability = UpdateAvailable;
			}
			else {

				updateInfo = nullptr;
				updateAvailability = NoUpdates;
			}
			updateAvailabilityChangedEvent(updateAvailability);
		}

		bool initialized{};
		ReleaseInfoCollection releases;

		UpdateCheckStatus status{ UpdateCheckStatus::Idle };
		winrt::Citrine::UpdateAvailability updateAvailability{ UpdateAvailability::Unknown };
		UpdateInfo updateInfo{ nullptr };
		Event<StatusChangedEventHandler> statusChangedEvent;
		Event<UpdateAvailabilityChangedEventHandler> updateAvailabilityChangedEvent;

		winrt::DispatcherQueue dispatcherQueue{ nullptr };
		winrt::ThreadPoolTimer refreshTimer{ nullptr };
		DateTime lastRefreshTime;

		EventToken automaticUpdateChecksChangedToken;
		EventToken updateChannelChangedToken;
		winrt::event_token networkConnectivityChangedToken;
	};

	UpdateCheckerInternal updateCheckerInternal;
}

namespace Citrine {

	auto UpdateChecker::InitializeAsync() -> void {

		updateCheckerInternal.InitializeAsync();
	}

	auto UpdateChecker::Status() noexcept -> winrt::Citrine::UpdateCheckStatus {

		return updateCheckerInternal.Status();
	}

	auto UpdateChecker::StatusChanged(StatusChangedEventHandler handler) -> EventToken {

		return updateCheckerInternal.StatusChanged(std::move(handler));
	}

	auto UpdateChecker::StatusChanged(EventToken&& token) -> void {

		updateCheckerInternal.StatusChanged(std::move(token));
	}

	auto UpdateChecker::UpdateAvailability() noexcept -> winrt::Citrine::UpdateAvailability {

		return updateCheckerInternal.UpdateAvailability();
	}

	auto UpdateChecker::UpdateAvailabilityChanged(UpdateAvailabilityChangedEventHandler handler) -> EventToken {

		return updateCheckerInternal.UpdateAvailabilityChanged(std::move(handler));
	}

	auto UpdateChecker::UpdateAvailabilityChanged(EventToken&& token) -> void {

		updateCheckerInternal.UpdateAvailabilityChanged(std::move(token));
	}

	auto UpdateChecker::CheckForUpdatesAsync() -> void {

		updateCheckerInternal.CheckForUpdatesAsync();
	}

	auto UpdateChecker::GetUpdateInfo() -> winrt::Citrine::UpdateInfo {

		return updateCheckerInternal.GetUpdateInfo();
	}
}