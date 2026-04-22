#pragma once

#include "MinecraftBedrockGameLaunchArgs.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGameLaunchArgs : MinecraftBedrockGameLaunchArgsT<MinecraftBedrockGameLaunchArgs>
    {
        MinecraftBedrockGameLaunchArgs(Citrine::MinecraftBedrockGamePackageItem const& selectedGamePackage);

        auto SelectedGamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;

        auto ActivateEditor() const noexcept -> bool;
        auto ActivateEditor(bool value) noexcept -> void;

        auto FileToImport() const noexcept -> winrt::hstring;
        auto FileToImport(winrt::hstring const& value) noexcept -> void;

    private:

        Citrine::MinecraftBedrockGamePackageItem selectedGamePackage{ nullptr };
        bool activateEditor{};
        winrt::hstring fileToImport;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGameLaunchArgs : MinecraftBedrockGameLaunchArgsT<MinecraftBedrockGameLaunchArgs, implementation::MinecraftBedrockGameLaunchArgs>
    {
    };
}
