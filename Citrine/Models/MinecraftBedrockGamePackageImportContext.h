#pragma once

#include "MinecraftBedrockGamePackageImportContext.g.h"

#include <filesystem>

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackageImportContext : MinecraftBedrockGamePackageImportContextT<MinecraftBedrockGamePackageImportContext>
    {
        MinecraftBedrockGamePackageImportContext(Citrine::MinecraftBedrockGamePackageItem gamePackage, std::filesystem::path gamePackageLocation);

        auto GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem;
        auto GamePackageLocation() const noexcept -> std::filesystem::path const&;

    private:

        Citrine::MinecraftBedrockGamePackageItem gamePackage{ nullptr };
        std::filesystem::path gamePackageLocation;
    };
}

/*
namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackageImportContext : MinecraftBedrockGamePackageImportContextT<MinecraftBedrockGamePackageImportContext, implementation::MinecraftBedrockGamePackageImportContext>
    {
    };
}
*/