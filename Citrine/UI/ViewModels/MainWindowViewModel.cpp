#include "pch.h"
#include "MainWindowViewModel.h"
#if __has_include("MainWindowViewModel.g.cpp")
#include "MainWindowViewModel.g.cpp"
#endif

#include "UI/Mvvm/RelayCommand.h"
#include "Services/ToastNotificationService.h"

using namespace Citrine;

namespace winrt::Citrine::implementation
{
	MainWindowViewModel::MainWindowViewModel() {

		notifications = winrt::make_self<ObservableCollection<Citrine::ToastNotification>>();

		notificationHandlerRevoker = ToastNotificationService::Subscribe([this](auto const& notification) {

			OnNotification(notification);
		});

		closeNotificationCommand = MakeRelayCommand([this](auto const& parameter) { 
			
			CloseNotification(parameter.as<Citrine::ToastNotification>());
		});
	}

	auto MainWindowViewModel::Notifications() const noexcept -> Citrine::IObservableCollectionView {

		return notifications->GetObservableView();
	}

	auto MainWindowViewModel::CloseNotificationCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand {

		return closeNotificationCommand;
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
}
