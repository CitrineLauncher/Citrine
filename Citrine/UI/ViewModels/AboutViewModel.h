#pragma once

#include "AboutViewModel.g.h"

namespace winrt::Citrine::implementation
{
    struct AboutViewModel : AboutViewModelT<AboutViewModel>
    {
        AboutViewModel();

        auto OpenHyperlinkCommand() const noexcept -> winrt::Microsoft::UI::Xaml::Input::ICommand;

        auto VersionInfoString() const noexcept -> winrt::hstring;
        auto RuntimeInfoString() const noexcept -> winrt::hstring;

    private:

        winrt::Microsoft::UI::Xaml::Input::ICommand openHyperlinkCommand;

        winrt::hstring versionInfoString;
        winrt::hstring runtimeInfoString;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct AboutViewModel : AboutViewModelT<AboutViewModel, implementation::AboutViewModel>
    {
    };
}
