#pragma once

#include "MinecraftJavaPage.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftJavaPage : MinecraftJavaPageT<MinecraftJavaPage>
    {
        MinecraftJavaPage() = default;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftJavaPage : MinecraftJavaPageT<MinecraftJavaPage, implementation::MinecraftJavaPage>
    {
    };
}
