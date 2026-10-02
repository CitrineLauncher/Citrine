#pragma once

#include "MainWindowViewModel.g.h"

#include "Core/Util/Event.h"
#include "Collections/ObservableCollection.h"
#include "Helpers/NotifyPropertyChangedBase.h"

namespace winrt::Citrine::implementation
{
    struct MainWindowViewModel : MainWindowViewModelT<MainWindowViewModel>, ::Citrine::NotifyPropertyChangedBase
    {
        MainWindowViewModel();

        auto Notifications() const noexcept -> Citrine::IObservableCollectionView;
        auto UpdateIsAvailable() const noexcept -> bool;

        auto CloseNotificationCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand;
        auto OpenReleasePageCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand;

    private:

        auto OnNotification(Citrine::ToastNotification const& notification) -> void;
        auto CloseNotification(Citrine::ToastNotification const& notification) -> void;

        auto OnAutomaticUpdateChecksChanged(bool automaticUpdateChecks) -> void;
        auto OnUpdateAvailabilityChanged(Citrine::UpdateAvailability availability) -> void;
        auto OpenReleasePage() -> winrt::fire_and_forget;

        static winrt::hstring const updateIsAvailableProperty;

        winrt::com_ptr<::Citrine::ObservableCollection<Citrine::ToastNotification>> notifications{ nullptr };
        ::Citrine::EventRevoker notificationHandlerRevoker;
        ::Citrine::EventRevoker automaticUpdateChecksChangedRevoker;
        ::Citrine::EventRevoker updateAvailabilityChangedRevoker;

        Citrine::IRelayCommand closeNotificationCommand{ nullptr };
        Citrine::IRelayCommand openReleasePageCommand{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MainWindowViewModel : MainWindowViewModelT<MainWindowViewModel, implementation::MainWindowViewModel>
    {
    };
}
