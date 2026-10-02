#pragma once

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct alignas(4) XvcSegment {

		std::uint32_t PageOffset;
		std::uint64_t Hash;
	};

#pragma pack(pop)
}