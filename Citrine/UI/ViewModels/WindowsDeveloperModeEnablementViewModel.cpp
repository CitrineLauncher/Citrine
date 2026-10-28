#include "pch.h"
#include "WindowsDeveloperModeEnablementViewModel.h"
#if __has_include("WindowsDeveloperModeEnablementViewModel.g.cpp")
#include "WindowsDeveloperModeEnablementViewModel.g.cpp"
#endif

#include "Services/PackageInstallationService.h"
#include "Services/ToastNotificationService.h"

#include "Locale/Localizer.h"

using namespace Citrine;

namespace winrt::Citrine::implementation
{
	auto WindowsDeveloperModeEnablementViewModel::GetWindowsDeveloperModeStatus() -> Citrine::WindowsDeveloperModeStatus {

		return PackageInstallationService::GetWindowsDeveloperModeStatus();
	}

	auto WindowsDeveloperModeEnablementViewModel::EnableWindowsDeveloperMode() -> winrt::Windows::Foundation::IAsyncOperation<Citrine::WindowsDeveloperModeStatus> {

		auto cancellationToken = co_await winrt::get_cancellation_token();
		cancellationToken.enable_propagation();

		auto status = co_await PackageInstallationService::EnableWindowsDeveloperModeAsync();
		if (status != WindowsDeveloperModeStatus::Enabled) {

			ToastNotificationService::SendNotification(NotificationSeverity::Error, Localizer::GetString(L"ErrorMessage_EnablingWindowsDeveloperModeFailed"), {});
		}
		co_return status;
	}
}
