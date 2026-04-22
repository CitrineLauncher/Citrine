#pragma once

#include "MinecraftBedrockPage.g.h"

#include <string_view>
#include <map>

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockPage : MinecraftBedrockPageT<MinecraftBedrockPage>
    {
        MinecraftBedrockPage() = default;

        auto InitializeComponent() -> void;
        auto NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args) -> void;

    private:

        std::map<std::wstring_view, Citrine::NavigationContext> navMap;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockPage : MinecraftBedrockPageT<MinecraftBedrockPage, implementation::MinecraftBedrockPage>
    {
    };
}
