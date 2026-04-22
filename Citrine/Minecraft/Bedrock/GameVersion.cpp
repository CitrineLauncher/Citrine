#include "pch.h"
#include "GameVersion.h"

namespace Citrine::Minecraft::Bedrock {

	auto GameVersion::FromWindowsAppPackageVersion(Windows::PackageVersion packageVersion) noexcept -> GameVersion {

		auto& [first, second, third, _] = packageVersion;
		auto version = GameVersion{};

		version.Baseline = first;

		if (second >= 100) {

			auto divisor = 1;
			while (second / divisor >= 100)
				divisor *= 10;

			version.Major = second / divisor;
			version.Minor = second % divisor;
			version.Revision = third;
			return version;
		}

		version.Major = second;
		version.Minor = third / 100;
		version.Revision = third % 100;
		return version;
	}
}