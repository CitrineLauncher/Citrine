#pragma once

#include "Windows/DeviceFamily.h"

#include <string>

namespace Citrine::Windows {

	struct MsixTargetDeviceFamily {

		std::string Name;
		DeviceFamilyVersion MinVersion;
		DeviceFamilyVersion MaxVersionTested;
	};
}