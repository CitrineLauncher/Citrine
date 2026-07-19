#pragma once

#include "Windows/AppModel.h"

#include <string>
#include <optional>

namespace Citrine::Windows {

	enum struct MsixPackageDependencyType : std::uint8_t {

		Install,
		InstallAndRuntime
	};

	struct MsixPackageDependency {

		std::string Name;
		std::string Publisher;
		PackageVersion MinVersion;
		std::optional<std::uint16_t> MaxMajorVersionTested;
		std::optional<bool> Optional;
		std::optional<MsixPackageDependencyType> Type;
	};

	struct MsixOSPackageDependency {

		std::string Name;
		PackageVersion Version;
	};

	struct MsixHostRuntimeDependency {

		std::string Name;
		std::string Publisher;
		PackageVersion MinVersion;
	};
}