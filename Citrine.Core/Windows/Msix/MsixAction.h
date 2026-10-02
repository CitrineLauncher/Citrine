#pragma once

#include <string>
#include <filesystem>

namespace Citrine::Windows {

	struct MsixActionBase {

		std::filesystem::path File;
		std::string Name;
		std::string Arguments;
	};

	struct MsixInstallAction : MsixActionBase {};
	struct MsixRepairAction : MsixActionBase {};
	struct MsixUninstallAction : MsixActionBase {};
}