#pragma once

#include "MinecraftBedrockServerDownloads.g.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockServerDownloads : MinecraftBedrockServerDownloadsT<MinecraftBedrockServerDownloads>
    {
        MinecraftBedrockServerDownloads(winrt::Windows::Foundation::Uri windowsPackage, winrt::Windows::Foundation::Uri linuxPackage);

        auto WindowsPackage() const noexcept -> winrt::Windows::Foundation::Uri;
        auto LinuxPackage() const noexcept -> winrt::Windows::Foundation::Uri;

    private:

        winrt::Windows::Foundation::Uri windowsPackage{ nullptr };
        winrt::Windows::Foundation::Uri linuxPackage{ nullptr };
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockServerDownloads : MinecraftBedrockServerDownloadsT<MinecraftBedrockServerDownloads, implementation::MinecraftBedrockServerDownloads>
    {
    };
}
*/
