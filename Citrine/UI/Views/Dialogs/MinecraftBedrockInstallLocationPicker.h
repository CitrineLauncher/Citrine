#pragma once

#include "MinecraftBedrockInstallLocationPicker.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockInstallLocationPicker : MinecraftBedrockInstallLocationPickerT<MinecraftBedrockInstallLocationPicker>
    {
        MinecraftBedrockInstallLocationPicker() = default;

        auto XamlRoot() const noexcept -> winrt::Microsoft::UI::Xaml::XamlRoot;
        auto XamlRoot(winrt::Microsoft::UI::Xaml::XamlRoot const& value) noexcept -> void;

        auto DefaultInstallLocation() const noexcept -> winrt::hstring;
        auto DefaultInstallLocation(winrt::hstring const& value) noexcept -> void;

        auto PickInstallLocation() -> winrt::hstring;

    private:

        winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot{ nullptr };
        winrt::hstring defaultInstallLocation;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockInstallLocationPicker : MinecraftBedrockInstallLocationPickerT<MinecraftBedrockInstallLocationPicker, implementation::MinecraftBedrockInstallLocationPicker>
    {
    };
}
