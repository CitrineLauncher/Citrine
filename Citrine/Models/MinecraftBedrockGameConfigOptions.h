#pragma once

#include "MinecraftBedrockGameConfigOptions.g.h"

#include "MinecraftBedrockGameConfigProvider.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGameConfigOptions : MinecraftBedrockGameConfigOptionsT<MinecraftBedrockGameConfigOptions>
    {
        MinecraftBedrockGameConfigOptions(implementation::MinecraftBedrockGameConfigProvider* provider);

        auto OptionIsAvailable(std::wstring_view optionName) -> bool;

        auto ModLoaderEnabled() const noexcept -> bool;
        auto ModLoaderEnabled(bool value) -> void;

    private:

        template<typename Self>
        auto GetConfig(this Self& self) -> auto& {

            return std::forward_like<Self&>(self.provider->config);
        }

        winrt::com_ptr<implementation::MinecraftBedrockGameConfigProvider> provider;
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGameConfigOptions : MinecraftBedrockGameConfigOptionsT<MinecraftBedrockGameConfigOptions, implementation::MinecraftBedrockGameConfigOptions>
    {
    };
}
*/
