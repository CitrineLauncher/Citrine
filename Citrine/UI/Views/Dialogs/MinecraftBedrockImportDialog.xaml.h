#pragma once

#include "UI/Controls/ContentDialogEx.h"
#include "MinecraftBedrockImportDialog.g.h"

#include "UI/Views/MinecraftBedrockGamePackageViewBase.h"
#include "UI/ViewModels/MinecraftBedrockGamePackagesViewModel.h"
#include "Models/MinecraftBedrockGamePackageImportContext.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockImportDialog : MinecraftBedrockImportDialogT<MinecraftBedrockImportDialog>, MinecraftBedrockGamePackageViewBase
    {
        MinecraftBedrockImportDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, winrt::hstring const& gamePackageLocation);

        auto ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel;

        auto GamePackage() const -> Citrine::MinecraftBedrockGamePackageItem;
        static auto GamePackageProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto PickInstallLocationButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args) -> void;

    private:

        auto OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&, winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args) -> void;

        auto EnsureProperties() -> void;

        auto InitiateImport(winrt::hstring const& gamePackageLocation) -> winrt::fire_and_forget;

        winrt::com_ptr<implementation::MinecraftBedrockGamePackagesViewModel> viewModel{ nullptr };
        winrt::Windows::Foundation::IAsyncOperation<Citrine::MinecraftBedrockGamePackageImportContext> initiateImportOp{ nullptr };
        winrt::com_ptr<implementation::MinecraftBedrockGamePackageImportContext> importContext{ nullptr };

        PrimaryButtonClick_revoker primaryButtonClickRevoker;

        static winrt::Microsoft::UI::Xaml::DependencyProperty gamePackageProperty;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockImportDialog : MinecraftBedrockImportDialogT<MinecraftBedrockImportDialog, implementation::MinecraftBedrockImportDialog>
    {
    };
}
