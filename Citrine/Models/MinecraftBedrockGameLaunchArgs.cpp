#include "pch.h"
#include "MinecraftBedrockGameLaunchArgs.h"
#if __has_include("MinecraftBedrockGameLaunchArgs.g.cpp")
#include "MinecraftBedrockGameLaunchArgs.g.cpp"
#endif

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGameLaunchArgs::MinecraftBedrockGameLaunchArgs(Citrine::MinecraftBedrockGamePackageItem const& selectedGamePackage)

        : selectedGamePackage(selectedGamePackage)
    {}

    auto MinecraftBedrockGameLaunchArgs::SelectedGamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

        return selectedGamePackage;
    }

    auto MinecraftBedrockGameLaunchArgs::ActivateEditor() const noexcept -> bool {

        return activateEditor;
    }

    auto MinecraftBedrockGameLaunchArgs::ActivateEditor(bool value) noexcept -> void {

        activateEditor = value;
    }

    auto MinecraftBedrockGameLaunchArgs::FileToImport() const noexcept -> winrt::hstring {

        return fileToImport;
    }

    auto MinecraftBedrockGameLaunchArgs::FileToImport(winrt::hstring const& value) noexcept -> void {

        fileToImport = value;
    }
}
