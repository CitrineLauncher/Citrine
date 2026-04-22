#pragma once

#include "MinecraftBedrockGamePackagesViewModel.g.h"

#include "Helpers/NotifyPropertyChangedBase.h"
#include "Core/Util/Event.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackagesViewModel : MinecraftBedrockGamePackagesViewModelT<MinecraftBedrockGamePackagesViewModel>, ::Citrine::NotifyPropertyChangedBase
    {
        MinecraftBedrockGamePackagesViewModel(winrt::hstring packageSourceId);

        auto GamePackages() const noexcept -> Citrine::IObservableCollectionView;
        auto FilteredGamePackages() const noexcept -> Citrine::IFilterableCollectionView;

        auto CurrentInstallLocation() const noexcept -> winrt::hstring;
        auto ValidateInstallLocation(winrt::hstring const& path) const noexcept -> Citrine::InstallLocationValidationResult;

        auto SupportsImporting() const noexcept -> bool;
        auto CanStartImporting() const noexcept -> bool;

        auto InstallGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage, winrt::hstring const& installLocation) -> void;
        auto InitiateGamePackageImport(winrt::hstring gamePackageLocation) -> winrt::Windows::Foundation::IAsyncOperation<Citrine::MinecraftBedrockGamePackageImportContext>;
        auto ImportGamePackage(Citrine::MinecraftBedrockGamePackageImportContext const& context, winrt::hstring const& installLocation, winrt::hstring const& nameTag) -> void;
        auto LaunchGamePackage(Citrine::MinecraftBedrockGameLaunchArgs launchArgs) -> winrt::fire_and_forget;
        auto RenameGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage, winrt::hstring const& nameTag) -> void;
        auto UninstallGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void;

        auto OpenGameDirectory(Citrine::MinecraftBedrockGamePackageItem gamePackage) -> winrt::fire_and_forget;
        auto OpenGameDataDirectory(Citrine::MinecraftBedrockGamePackageItem gamePackage) -> winrt::fire_and_forget;

        auto PauseGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void;
        auto ResumeGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void;
        auto CancelGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void;

        auto PackageSourceId() const noexcept -> winrt::hstring;

    private:

        Citrine::IObservableCollectionView gamePackages{ nullptr };
        Citrine::IFilterableCollectionView filteredGamePackages{ nullptr };
        winrt::hstring packageSourceId;

        bool canStartImporting{};
        ::Citrine::EventRevoker gameManagerInitializationCompletedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackagesViewModel : MinecraftBedrockGamePackagesViewModelT<MinecraftBedrockGamePackagesViewModel, implementation::MinecraftBedrockGamePackagesViewModel>
    {
    };
}
