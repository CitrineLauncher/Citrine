#pragma once

#include "WindowsDeveloperModeEnablementViewModel.g.h"

namespace winrt::Citrine::implementation
{
    struct WindowsDeveloperModeEnablementViewModel : WindowsDeveloperModeEnablementViewModelT<WindowsDeveloperModeEnablementViewModel>
    {
        WindowsDeveloperModeEnablementViewModel() = default;

        auto GetWindowsDeveloperModeStatus() -> Citrine::WindowsDeveloperModeStatus;
        auto EnableWindowsDeveloperMode() -> winrt::Windows::Foundation::IAsyncOperation<Citrine::WindowsDeveloperModeStatus>;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct WindowsDeveloperModeEnablementViewModel : WindowsDeveloperModeEnablementViewModelT<WindowsDeveloperModeEnablementViewModel, implementation::WindowsDeveloperModeEnablementViewModel>
    {
    };
}
