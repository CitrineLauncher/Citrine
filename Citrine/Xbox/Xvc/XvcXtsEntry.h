#pragma once

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct alignas(4) XvcXtsEntry {

		std::uint32_t PageOffset;
		std::uint32_t XtsOffset;
	};

#pragma pack(pop)
}