#pragma once

#include <cstdint>

namespace Citrine {

#pragma pack(push, 1)

	struct ZipExtraFieldHeader {

		std::uint16_t Id;
		std::uint16_t Size;
	};

#pragma pack(pop)
}