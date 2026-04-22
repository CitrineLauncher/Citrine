#pragma once

#include "XvcRegionId.h"

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct alignas(4) XvcRegionSpecifier {

		XvcRegionId RegionId;
		std::uint32_t Flags;
		wchar_t Key[0x40];
		wchar_t Value[0x80];
	};

#pragma pack(pop)
}