#pragma once

#include "UI/Controls/ContentDialogEx.h"
#include "MinecraftBedrockManageDialog.g.h"

#include "UI/Views/MinecraftBedrockGamePackageViewBase.h"
#include "UI/ViewModels/MinecraftBedrockGamePackagesViewModel.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockManageDialog : MinecraftBedrockManageDialogT<MinecraftBedrockManageDialog>, MinecraftBedrockGamePackageViewBase
    {
        MinecraftBedrockManageDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage);

        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel;
        auto GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;

        auto CloseButton2_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args) -> void;
        auto OpenGameDirectoryButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args) -> void;
        auto UninstallButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args) -> void;

    private:

        winrt::com_ptr<implementation::MinecraftBedrockGamePackagesViewModel> viewModel{ nullptr };
        Citrine::MinecraftBedrockGamePackageItem gamePackage{ nullptr };

        PrimaryButtonClick_revoker primaryButtonClickRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockManageDialog : MinecraftBedrockManageDialogT<MinecraftBedrockManageDialog, implementation::MinecraftBedrockManageDialog>
    {
    };
}
