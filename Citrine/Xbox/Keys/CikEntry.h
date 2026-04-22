#pragma once

#include "Core/Util/Guid.h"

namespace Citrine::Xbox {

	struct CikEntry {

		Guid Id;
		std::uint8_t TweakKey[0x10];
		std::uint8_t DataKey[0x10];
	};
}