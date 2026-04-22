#include "pch.h"
#include "MinecraftBedrockGamePackageImportContext.h"
#if __has_include("MinecraftBedrockGamePackageImportContext.g.cpp")
#include "MinecraftBedrockGamePackageImportContext.g.cpp"
#endif

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGamePackageImportContext::MinecraftBedrockGamePackageImportContext(Citrine::MinecraftBedrockGamePackageItem gamePackage, std::filesystem::path gamePackageLocation)

        : gamePackage(std::move(gamePackage))
        , gamePackageLocation(std::move(gamePackageLocation))
    {}

    auto MinecraftBedrockGamePackageImportContext::GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

        return gamePackage;
    }

    auto MinecraftBedrockGamePackageImportContext::GamePackageLocation() const noexcept -> std::filesystem::path const& {

        return gamePackageLocation;
    }
}
