#include "pch.h"
#include "GamePackageMeta.h"

using namespace Citrine::Windows;

namespace Citrine::Minecraft::Bedrock {

	auto CheckGamePackageCompatibility([[maybe_unused]] GameVersion version, GamePlatform platform, PackageArchitecture architecture) noexcept -> GamePackageCompatibility {

		auto score = std::uint8_t{};

#if defined(_M_X64)
		switch (architecture)	 {
		case PackageArchitecture::X64:	score = 0xF0;	break;
		case PackageArchitecture::X86:	score = 0xE0;	break;
		default: return {};
		}

		switch (platform) {
		case GamePlatform::WindowsGDK:	score |= 0x0F;	break;
		case GamePlatform::WindowsUWP:	score |= 0x0E;	break;
		case GamePlatform::XboxUWP:		score |= 0x0D;	break;
		default: return {};
		}
#endif
		return { score };
	}
}