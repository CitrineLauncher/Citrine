#include "pch.h"
#include "MinecraftBedrockGameConfigOptions.h"
#if __has_include("MinecraftBedrockGameConfigOptions.g.cpp")
#include "MinecraftBedrockGameConfigOptions.g.cpp"
#endif

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGameConfigOptions::MinecraftBedrockGameConfigOptions(implementation::MinecraftBedrockGameConfigProvider* provider)

        : provider(provider->get_strong())
    {}

    auto MinecraftBedrockGameConfigOptions::OptionIsAvailable(std::wstring_view optionName) -> bool {

        return provider->CheckOptionAvailability(optionName);
    }

    auto MinecraftBedrockGameConfigOptions::ModLoaderEnabled() const noexcept -> bool {

        auto& config = GetConfig();
        return config.ModLoaderEnabled;
    }

    auto MinecraftBedrockGameConfigOptions::ModLoaderEnabled(bool value) -> void {

        auto& config = GetConfig();
        if (config.ModLoaderEnabled != value) {

            config.ModLoaderEnabled = value;
            provider->OnConfigChanged();
        }
    }
}
