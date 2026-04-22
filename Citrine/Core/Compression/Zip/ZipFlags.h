#pragma once

#include <cstdint>
#include <utility>

namespace Citrine {

	enum struct ZipFlags : std::uint16_t {

		Encrypted = 0x0001,
		CompressionOption1 = 0x0002,
		CompressionOption2 = 0x0004,
		DataDescriptor = 0x0008,
		StrongEncryption = 0x0040,
		Utf8 = 0x0800
	};

	constexpr auto operator|(ZipFlags left, ZipFlags right) noexcept -> ZipFlags {

		return static_cast<ZipFlags>(std::to_underlying(left) | std::to_underlying(right));
	}

	constexpr auto operator&(ZipFlags left, ZipFlags right) noexcept -> ZipFlags {

		return static_cast<ZipFlags>(std::to_underlying(left) & std::to_underlying(right));
	}
}