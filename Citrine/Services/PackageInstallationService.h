#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Util/Param.h"
#include "Windows/AppModel.h"

namespace Citrine {

	class PackageInstallationService {
	public:

		static auto InstallPackageAsync(StringParameter packageFamilyName) -> Task<bool>;
	};
}