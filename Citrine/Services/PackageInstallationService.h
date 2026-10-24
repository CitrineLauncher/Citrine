#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Util/Param.h"
#include "Windows/AppModel.h"

#include "winrt/Citrine.h"

namespace Citrine {

	class PackageInstallationService {
	public:

		static auto GetWindowsDeveloperModeStatus() -> winrt::Citrine::WindowsDeveloperModeStatus;
		static auto EnableWindowsDeveloperModeAsync() -> Task<winrt::Citrine::WindowsDeveloperModeStatus>;

		static auto InstallPackageAsync(StringParameter packageFamilyName) -> Task<bool>;
	};
}