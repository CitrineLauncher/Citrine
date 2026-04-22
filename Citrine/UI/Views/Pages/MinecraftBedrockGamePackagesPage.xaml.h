#pragma once

#include "MinecraftBedrockGamePackagesPage.g.h"

#include "UI/ViewModels/MinecraftBedrockGamePackagesViewModel.h"

#include <string_view>

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackagesPage : MinecraftBedrockGamePackagesPageT<MinecraftBedrockGamePackagesPage>
    {
        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel;

        auto InvokeGamePackageActionCommand_ExecuteRequested(winrt::Microsoft::UI::Xaml::Input::XamlUICommand const&, winrt::Microsoft::UI::Xaml::Input::ExecuteRequestedEventArgs args) -> winrt::fire_and_forget;
        auto ImportButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> winrt::fire_and_forget;

    protected:

        MinecraftBedrockGamePackagesPage(winrt::hstring packageSourceId);

        auto OnLoaded() -> void;
        auto OnSizeChanged(double newWidth, double newHeight) -> void;

        winrt::com_ptr<implementation::MinecraftBedrockGamePackagesViewModel> viewModel{ nullptr };
        winrt::Microsoft::UI::Xaml::FrameworkElement contentArea{ nullptr };
        double baseContentHeight{};

        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged::CollectionChanged_revoker gamePackagesChangedRevoker;
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackagesPage : MinecraftBedrockGamePackagesPageT<MinecraftBedrockGamePackagesPage, implementation::MinecraftBedrockGamePackagesPage>
    {
    };
}
*/
