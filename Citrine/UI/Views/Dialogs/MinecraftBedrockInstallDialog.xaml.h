#pragma once

#include "UI/Controls/ContentDialogEx.h"
#include "MinecraftBedrockInstallDialog.g.h"

#include "UI/Views/MinecraftBedrockGamePackageViewBase.h"
#include "UI/ViewModels/MinecraftBedrockGamePackagesViewModel.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockInstallDialog : MinecraftBedrockInstallDialogT<MinecraftBedrockInstallDialog>, MinecraftBedrockGamePackageViewBase
    {
        MinecraftBedrockInstallDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage);

        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel;
        auto GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;

        auto PickInstallLocationButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args) -> void;

    private:

        auto OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&, winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args) -> void;

        winrt::com_ptr<implementation::MinecraftBedrockGamePackagesViewModel> viewModel{ nullptr };
        Citrine::MinecraftBedrockGamePackageItem gamePackage{ nullptr };

        PrimaryButtonClick_revoker primaryButtonClickRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockInstallDialog : MinecraftBedrockInstallDialogT<MinecraftBedrockInstallDialog, implementation::MinecraftBedrockInstallDialog>
    {
    };
}
