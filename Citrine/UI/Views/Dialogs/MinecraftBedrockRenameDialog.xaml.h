#pragma once

#include "UI/Controls/ContentDialogEx.h"
#include "MinecraftBedrockRenameDialog.g.h"

#include "UI/Views/MinecraftBedrockGamePackageViewBase.h"
#include "UI/ViewModels/MinecraftBedrockGamePackagesViewModel.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockRenameDialog : MinecraftBedrockRenameDialogT<MinecraftBedrockRenameDialog>, MinecraftBedrockGamePackageViewBase
    {
        MinecraftBedrockRenameDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage);

        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel;
        auto GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;

        auto OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&, winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args) -> void;

    private:

        winrt::com_ptr<implementation::MinecraftBedrockGamePackagesViewModel> viewModel{ nullptr };
        Citrine::MinecraftBedrockGamePackageItem gamePackage{ nullptr };

        PrimaryButtonClick_revoker primaryButtonClickRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockRenameDialog : MinecraftBedrockRenameDialogT<MinecraftBedrockRenameDialog, implementation::MinecraftBedrockRenameDialog>
    {
    };
}
