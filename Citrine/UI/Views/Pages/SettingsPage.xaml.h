#pragma once

#include "SettingsPage.g.h"

#include <string_view>
#include <map>

namespace winrt::Citrine::implementation
{
    struct SettingsPage : SettingsPageT<SettingsPage>
    {
        SettingsPage() = default;
        
        auto InitializeComponent() -> void;
        auto NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args) -> void;

    private:

        std::map<std::wstring_view, Citrine::NavigationContext> navMap;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage>
    {
    };
}
