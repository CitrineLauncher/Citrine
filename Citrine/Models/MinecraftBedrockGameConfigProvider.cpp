#include "pch.h"
#include "MinecraftBedrockGameConfigProvider.h"
#if __has_include("MinecraftBedrockGameConfigProvider.g.cpp")
#include "MinecraftBedrockGameConfigProvider.g.cpp"
#endif

#include "MinecraftBedrockGameConfigOptions.h"

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGameConfigProvider::MinecraftBedrockGameConfigProvider(
        Citrine::MinecraftBedrockGamePackageItem targetGamePackage,
        ::Citrine::Minecraft::Bedrock::GameConfig&& config,
        ConfigChangedCallback configChangedCallback,
        CheckOptionAvailabilityFunc checkOptionAvailabilityFunc
    )
        : targetGamePackage(std::move(targetGamePackage))
        , config(std::move(config))
        , configChangedCallback(std::move(configChangedCallback))
        , checkOptionAvailabilityFunc(std::move(checkOptionAvailabilityFunc))
    {}

    auto MinecraftBedrockGameConfigProvider::TargetGamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

        return targetGamePackage;
    }

    auto MinecraftBedrockGameConfigProvider::GetOptions() -> Citrine::MinecraftBedrockGameConfigOptions {

        return winrt::make<implementation::MinecraftBedrockGameConfigOptions>(this);
    }

    auto MinecraftBedrockGameConfigProvider::OnConfigChanged() -> void {

        if (configChangedCallback)
            configChangedCallback(targetGamePackage, config);
    }

    auto MinecraftBedrockGameConfigProvider::CheckOptionAvailability(std::wstring_view optionName) -> bool {

        if (checkOptionAvailabilityFunc)
            return checkOptionAvailabilityFunc(targetGamePackage, optionName);

        return true;
    }
}
