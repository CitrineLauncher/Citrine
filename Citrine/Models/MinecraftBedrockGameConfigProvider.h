#pragma once

#include "MinecraftBedrockGameConfigProvider.g.h"

#include "Minecraft/Bedrock/GameConfig.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGameConfigOptions;

    struct MinecraftBedrockGameConfigProvider : MinecraftBedrockGameConfigProviderT<MinecraftBedrockGameConfigProvider>
    {
        using ConfigChangedCallback = winrt::delegate<Citrine::MinecraftBedrockGamePackageItem, ::Citrine::Minecraft::Bedrock::GameConfig>;
        using CheckOptionAvailabilityFunc = winrt::delegate<auto(Citrine::MinecraftBedrockGamePackageItem, std::wstring_view) -> bool>;

        MinecraftBedrockGameConfigProvider(
            Citrine::MinecraftBedrockGamePackageItem targetGamePackage,
            ::Citrine::Minecraft::Bedrock::GameConfig&& config,
            ConfigChangedCallback configChangedCallback,
            CheckOptionAvailabilityFunc checkOptionAvailabilityFunc
        );

        auto TargetGamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;
        auto GetOptions() -> Citrine::MinecraftBedrockGameConfigOptions;

    private:

        auto OnConfigChanged() -> void;
        auto CheckOptionAvailability(std::wstring_view optionName) -> bool;

        friend MinecraftBedrockGameConfigOptions;

        Citrine::MinecraftBedrockGamePackageItem targetGamePackage;
        ::Citrine::Minecraft::Bedrock::GameConfig config;
        ConfigChangedCallback configChangedCallback;
        CheckOptionAvailabilityFunc checkOptionAvailabilityFunc;
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGameConfigProvider : MinecraftBedrockGameConfigProviderT<MinecraftBedrockGameConfigProvider, implementation::MinecraftBedrockGameConfigProvider>
    {
    };
}
*/
