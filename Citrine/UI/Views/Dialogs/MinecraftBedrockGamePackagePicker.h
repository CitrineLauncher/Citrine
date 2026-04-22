#pragma once

#include "MinecraftBedrockGamePackagePicker.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackagePicker : MinecraftBedrockGamePackagePickerT<MinecraftBedrockGamePackagePicker>
    {
        MinecraftBedrockGamePackagePicker() = default;

        auto XamlRoot() const noexcept -> winrt::Microsoft::UI::Xaml::XamlRoot;
        auto XamlRoot(winrt::Microsoft::UI::Xaml::XamlRoot const& value) noexcept -> void;

        auto PickGamePackage() -> winrt::hstring;

    private:

        winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackagePicker : MinecraftBedrockGamePackagePickerT<MinecraftBedrockGamePackagePicker, implementation::MinecraftBedrockGamePackagePicker>
    {
    };
}
