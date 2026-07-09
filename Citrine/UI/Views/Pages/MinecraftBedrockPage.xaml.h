#pragma once

#include "MinecraftBedrockPage.g.h"

#include "UI/ViewModels/MinecraftBedrockViewModel.h";

#include <string_view>
#include <map>

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockPage : MinecraftBedrockPageT<MinecraftBedrockPage>
    {
        MinecraftBedrockPage();

        auto InitializeComponent() -> void;

        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockViewModel;

        auto NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args) -> void;

    private:

        auto OnLoaded() -> void;
        auto OnSizeChanged(double newWidth, double newHeight) -> void;

        winrt::com_ptr<implementation::MinecraftBedrockViewModel> viewModel = winrt::make_self<implementation::MinecraftBedrockViewModel>();
        std::map<std::wstring_view, Citrine::NavigationContext> navMap;
        winrt::Microsoft::UI::Xaml::Controls::Page activePage{ nullptr };
        double viewModeToggleWidth{};
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockPage : MinecraftBedrockPageT<MinecraftBedrockPage, implementation::MinecraftBedrockPage>
    {
    };
}
