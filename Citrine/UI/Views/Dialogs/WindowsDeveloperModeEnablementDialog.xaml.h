#pragma once

#include "UI/Controls/ContentDialogEx.h"
#include "WindowsDeveloperModeEnablementDialog.g.h"

#include "UI/ViewModels/WindowsDeveloperModeEnablementViewModel.h"

namespace winrt::Citrine::implementation
{
    struct WindowsDeveloperModeEnablementDialog : WindowsDeveloperModeEnablementDialogT<WindowsDeveloperModeEnablementDialog>
    {
        WindowsDeveloperModeEnablementDialog(Citrine::WindowsDeveloperModeEnablementViewModel const& viewModel);

        auto ViewModel() const noexcept -> Citrine::WindowsDeveloperModeEnablementViewModel;

    private:

        auto OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&, winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args) -> winrt::fire_and_forget;

        winrt::com_ptr<implementation::WindowsDeveloperModeEnablementViewModel> viewModel{ nullptr };
        winrt::Windows::Foundation::IAsyncOperation<Citrine::WindowsDeveloperModeStatus> enableOp{ nullptr };

        PrimaryButtonClick_revoker primaryButtonClickRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct WindowsDeveloperModeEnablementDialog : WindowsDeveloperModeEnablementDialogT<WindowsDeveloperModeEnablementDialog, implementation::WindowsDeveloperModeEnablementDialog>
    {
    };
}
