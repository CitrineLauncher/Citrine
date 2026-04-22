#include "pch.h"
#include "GameCapabilities.h"

namespace Citrine::Minecraft::Bedrock {

	auto CheckGameCapabilities(GameVersion version, GameBuildType buildType, GamePlatform platform, Windows::PackageArchitecture architecture) noexcept -> GameCapabilities {

		using enum GameCapabilities;
		auto capabilities = GameCapabilities{};

		{
			using enum GamePlatform;
			if (platform != WindowsGDK && platform != WindowsUWP)
				return capabilities;
		}

		{
			using enum GameBuildType;
			auto supportsEditor =
				(buildType == Release && version >= GameVersion{ 1, 21, 50, 7 }) ||
				(buildType == Preview && version >= GameVersion{ 1, 19, 80, 20 });

			if (supportsEditor)
				capabilities = capabilities | Editor;
		}

		return capabilities;
	}
}