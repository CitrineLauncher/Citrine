#pragma once

#include "MinecraftBedrockViewModel.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockViewModel : MinecraftBedrockViewModelT<MinecraftBedrockViewModel>
    {
        MinecraftBedrockViewModel() = default;

        auto PackageViewMode() const noexcept -> std::int32_t;
        auto PackageViewMode(std::int32_t value) -> void;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockViewModel : MinecraftBedrockViewModelT<MinecraftBedrockViewModel, implementation::MinecraftBedrockViewModel>
    {
    };
}
