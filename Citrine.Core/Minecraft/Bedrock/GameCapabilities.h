#pragma once

#include "GameVersion.h"
#include "GameBuildType.h"
#include "GamePlatform.h"

#include "Windows/AppModel.h"

#include <utility>

namespace Citrine::Minecraft::Bedrock {

	enum struct GameCapabilities : std::uint8_t {

		None			= 0,
		Editor			= 1 << 0,
		VibrantVisuals	= 1 << 1,
		RayTracing		= 1 << 2
	};

	constexpr auto operator|(GameCapabilities left, GameCapabilities right) noexcept -> GameCapabilities {

		return static_cast<GameCapabilities>(std::to_underlying(left) | std::to_underlying(right));
	}

	constexpr auto operator&(GameCapabilities left, GameCapabilities right) noexcept -> GameCapabilities {

		return static_cast<GameCapabilities>(std::to_underlying(left) & std::to_underlying(right));
	}

	auto CheckGameCapabilities(GameVersion version, GameBuildType buildType, GamePlatform platform, Windows::PackageArchitecture architecture) noexcept -> GameCapabilities;

	auto CheckGameCapabilities(auto const& package) noexcept -> GameCapabilities {

		return CheckGameCapabilities(package.Version, package.BuildType, package.Platform, package.Architecture);
	}
}
