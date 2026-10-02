#pragma once

#include <cstdint>

namespace Citrine {

	enum struct ZipCompressionMethod : std::uint16_t {

		None = 0,
		Deflate = 8
	};
}