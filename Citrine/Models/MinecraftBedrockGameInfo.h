#pragma once

#include "MinecraftBedrockGameInfo.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGameInfo
    {
        static auto AssociatedFileTypes() -> winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring>;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGameInfo : MinecraftBedrockGameInfoT<MinecraftBedrockGameInfo, implementation::MinecraftBedrockGameInfo>
    {
    };
}
