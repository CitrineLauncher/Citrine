#pragma once

#include "MainWindowViewModel.g.h"

#include "Core/Util/Event.h"
#include "Collections/ObservableCollection.h"

namespace winrt::Citrine::implementation
{
    struct MainWindowViewModel : MainWindowViewModelT<MainWindowViewModel>
    {
        MainWindowViewModel();

        auto Notifications() const noexcept -> Citrine::IObservableCollectionView;

        auto CloseNotificationCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand;

    private:

        auto OnNotification(Citrine::ToastNotification const& notification) -> void;
        auto CloseNotification(Citrine::ToastNotification const& notification) -> void;

        winrt::com_ptr<::Citrine::ObservableCollection<Citrine::ToastNotification>> notifications{ nullptr };
        ::Citrine::EventRevoker notificationHandlerRevoker;
        Citrine::IRelayCommand closeNotificationCommand{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MainWindowViewModel : MainWindowViewModelT<MainWindowViewModel, implementation::MainWindowViewModel>
    {
    };
}
