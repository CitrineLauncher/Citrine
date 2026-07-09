#include "pch.h"
#include "MinecraftBedrockViewModel.h"
#if __has_include("MinecraftBedrockViewModel.g.cpp")
#include "MinecraftBedrockViewModel.g.cpp"
#endif

#include "ApplicationData.h"

using namespace Citrine;

namespace winrt::Citrine::implementation
{
	auto MinecraftBedrockViewModel::PackageViewMode() const noexcept -> std::int32_t {

		auto& settings = ApplicationData::LocalSettings();
		return std::to_underlying(settings.PackageViewMode());
	}

	auto MinecraftBedrockViewModel::PackageViewMode(std::int32_t value) -> void {

		auto& settings = ApplicationData::LocalSettings();
		settings.PackageViewMode(static_cast<::Citrine::PackageViewMode>(value));
	}
}
