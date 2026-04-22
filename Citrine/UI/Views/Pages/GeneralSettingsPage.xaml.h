#pragma once

#include "GeneralSettingsPage.g.h"

#include "UI/ViewModels/GeneralSettingsViewModel.h"

namespace winrt::Citrine::implementation
{
    struct GeneralSettingsPage : GeneralSettingsPageT<GeneralSettingsPage>
    {
        GeneralSettingsPage();

        auto ViewModel() const noexcept -> Citrine::GeneralSettingsViewModel;

    private:

        auto OnLoaded() -> void;
        auto OnSizeChanged(double newWidth, double newHeight) -> void;

        winrt::com_ptr<implementation::GeneralSettingsViewModel> viewModel = winrt::make_self<implementation::GeneralSettingsViewModel>();
        winrt::Microsoft::UI::Xaml::FrameworkElement contentArea{ nullptr };
        double baseContentHeight{};
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct GeneralSettingsPage : GeneralSettingsPageT<GeneralSettingsPage, implementation::GeneralSettingsPage>
    {
    };
}
