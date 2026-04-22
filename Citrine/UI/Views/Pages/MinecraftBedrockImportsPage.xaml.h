#pragma once

#include "MinecraftBedrockGamePackagesPage.xaml.h"
#include "MinecraftBedrockImportsPage.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockImportsPage : MinecraftBedrockImportsPageT<MinecraftBedrockImportsPage>
    {
        MinecraftBedrockImportsPage();

        auto InitializeComponent() -> void;
        auto Connect(std::int32_t connectionId, winrt::Windows::Foundation::IInspectable const& target) -> void override;
        auto GetBindingConnector(std::int32_t connectionId, winrt::Windows::Foundation::IInspectable const& target) -> winrt::Microsoft::UI::Xaml::Markup::IComponentConnector override;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockImportsPage : MinecraftBedrockImportsPageT<MinecraftBedrockImportsPage, implementation::MinecraftBedrockImportsPage>
    {
    };
}
