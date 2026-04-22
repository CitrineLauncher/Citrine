#pragma once

#include "MinecraftBedrockGamePackageItem.g.h"

#include "Helpers/NotifyPropertyChangedBase.h"
#include "Minecraft/Bedrock/GamePackage.h"
#include "Minecraft/Bedrock/GamePackageMeta.h"
#include "Minecraft/Bedrock/GameCapabilities.h"

#include <compare>

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackageItem : MinecraftBedrockGamePackageItemT<MinecraftBedrockGamePackageItem>, ::Citrine::NotifyPropertyChangedBase
    {
        MinecraftBedrockGamePackageItem(
            ::Citrine::Minecraft::Bedrock::GamePackageIdentity const& id,
            winrt::hstring nameTag,
            ::Citrine::Minecraft::Bedrock::GamePackageCompatibility compatibility,
            bool installed,
            ::Citrine::Minecraft::Bedrock::GameCapabilities gameCapabilities,
            Citrine::MinecraftBedrockGamePackageStatus status
        );

        auto Version() const noexcept -> winrt::hstring;
        auto BuildType() const -> winrt::hstring;
        auto Platform() const -> winrt::hstring;
        auto Architecture() const noexcept -> winrt::Windows::System::ProcessorArchitecture;
        auto Origin() const -> winrt::hstring;
        auto InstallId() const noexcept -> std::uint16_t;

        auto NormalizedVersion() const noexcept -> winrt::hstring;
        auto NameTag() const noexcept -> winrt::hstring;
        auto NameTag(winrt::hstring const& value) -> void;

        auto IsSupported() const noexcept -> bool;
        auto IsRenamable() const noexcept -> bool;

        auto IsInstalled() const noexcept -> bool;
        auto IsInstalled(bool value) -> void;

        auto Status() const noexcept -> Citrine::MinecraftBedrockGamePackageStatus;
        auto Status(Citrine::MinecraftBedrockGamePackageStatus value) -> void;

        auto OperationProgress() const noexcept -> Citrine::ByteProgress;
        auto OperationProgress(Citrine::ByteProgress const& value) -> void;
        auto OperationProgressIsIndeterminate() const noexcept -> bool;

        auto OperationIsPausable() const noexcept -> bool;
        auto OperationIsCancellable() const noexcept -> bool;

        auto GameSupportsEditor() const noexcept -> bool;
        auto GameSupportsVibrantVisuals() const noexcept -> bool;
        auto GameSupportsRayTracing() const noexcept -> bool;
        auto GameAssociatedFileTypes() const noexcept -> winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring>;

        auto Id() const noexcept -> ::Citrine::Minecraft::Bedrock::GamePackageIdentity const&;
        auto Compatibility() const noexcept -> ::Citrine::Minecraft::Bedrock::GamePackageCompatibility;
        auto GameCapabilities() const noexcept -> ::Citrine::Minecraft::Bedrock::GameCapabilities;

        auto CompareTo(MinecraftBedrockGamePackageItem const& other) const noexcept -> std::strong_ordering;

    private:

        static winrt::hstring const nameTagProperty;
        static winrt::hstring const isInstalledProperty;
        static winrt::hstring const statusProperty;
        static winrt::hstring const operationProgressProperty;
        static winrt::hstring const operationProgressIsIndeterminateProperty;
        static winrt::hstring const operationIsPausableProperty;
        static winrt::hstring const operationIsCancellableProperty;

        ::Citrine::Minecraft::Bedrock::GamePackageIdentity id;
        winrt::hstring version;
        winrt::hstring normalizedVersion;
        winrt::hstring nameTag;
        ::Citrine::Minecraft::Bedrock::GamePackageCompatibility compatibility;
        bool installed{};
        ::Citrine::Minecraft::Bedrock::GameCapabilities gameCapabilities;
        Citrine::MinecraftBedrockGamePackageStatus status{};
        Citrine::ByteProgress operationProgress;
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackageItem : MinecraftBedrockGamePackageItemT<MinecraftBedrockGamePackageItem, implementation::MinecraftBedrockGamePackageItem>
    {
    };
}
*/
