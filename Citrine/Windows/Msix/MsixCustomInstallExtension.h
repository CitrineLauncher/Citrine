#pragma once

#include "MsixAction.h"

#include <filesystem>

namespace Citrine::Windows {

	struct MsixCustomInstallExtension {

		std::filesystem::path FileDirectory;
		bool RunAsUser{};
		std::vector<MsixInstallAction> InstallActions;
		std::vector<MsixRepairAction> RepairActions;
		std::vector<MsixUninstallAction> UninstallActions;
	};
}