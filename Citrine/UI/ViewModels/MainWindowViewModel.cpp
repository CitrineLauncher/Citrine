#include "pch.h"
#include "MainWindowViewModel.h"
#if __has_include("MainWindowViewModel.g.cpp")
#include "MainWindowViewModel.g.cpp"
#endif

#include "UI/Mvvm/RelayCommand.h"
#include "Services/ToastNotificationService.h"
#include "Services/UpdateChecker.h"
#include "ApplicationData.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::System;
}

namespace winrt::Citrine::implementation
{
	MainWindowViewModel::MainWindowViewModel() {

		auto& appSettings = ApplicationData::LocalSettings();

		notifications = winrt::make_self<ObservableCollection<Citrine::ToastNotification>>();

		notificationHandlerRevoker = ToastNotificationService::Subscribe({ this, &MainWindowViewModel::OnNotification });
		automaticUpdateChecksChangedRevoker = appSettings.AutomaticUpdateChecksChanged({ this, &MainWindowViewModel::OnAutomaticUpdateChecksChanged });
		updateAvailabilityChangedRevoker = UpdateChecker::UpdateAvailabilityChanged({ this, &MainWindowViewModel::OnUpdateAvailabilityChanged });

		closeNotificationCommand = MakeRelayCommand([this](auto const& parameter) { 
			
			CloseNotification(parameter.as<Citrine::ToastNotification>());
		});

		openReleasePageCommand = MakeRelayCommand([this] {

			OpenReleasePage();
		});
	}

	auto MainWindowViewModel::Notifications() const noexcept -> Citrine::IObservableCollectionView {

		return notifications->GetObservableView();
	}

	auto MainWindowViewModel::UpdateIsAvailable() const noexcept -> bool {

		using enum Citrine::UpdateAvailability;

		auto& appSettings = ApplicationData::LocalSettings();

		auto availability = UpdateChecker::UpdateAvailability();
		auto automaticUpdateChecks = appSettings.AutomaticUpdateChecks();

		return (availability == UpdateAvailable || availability == UpdateRequired) && automaticUpdateChecks;
	}

	auto MainWindowViewModel::CloseNotificationCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand {

		return closeNotificationCommand;
	}

	auto MainWindowViewModel::OpenReleasePageCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand {

		return openReleasePageCommand;
	}

	auto MainWindowViewModel::OnNotification(Citrine::ToastNotification const& notification) -> void {

		if (notifications->Size() >= 6)
			notifications->RemoveAtEnd();
		notifications->InsertAt(0, notification);
	}

	auto MainWindowViewModel::CloseNotification(Citrine::ToastNotification const& notification) -> void {

		auto index = std::uint32_t{};
		if (notifications->IndexOf(notification, index)) {

			notifications->RemoveAt(index);
		}
	}

	auto MainWindowViewModel::OnAutomaticUpdateChecksChanged(bool automaticUpdateChecks) -> void {

		OnPropertyChanged(updateIsAvailableProperty);
	}

	auto MainWindowViewModel::OnUpdateAvailabilityChanged(Citrine::UpdateAvailability availability) -> void {

		OnPropertyChanged(updateIsAvailableProperty);
	}

	auto MainWindowViewModel::OpenReleasePage() -> winrt::fire_and_forget try {

		auto updateInfo = UpdateChecker::GetUpdateInfo();
		if (!updateInfo)
			co_return;

		co_await winrt::Launcher::LaunchUriAsync(updateInfo.ReleaseUrl());
	}
	catch (winrt::hresult_error const&) {}

	winrt::hstring const MainWindowViewModel::updateIsAvailableProperty = L"UpdateIsAvailable";
}
