#pragma once

#include "Core/Util/Guid.h"

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct XvdSegmentMetadataHeader {

		std::uint32_t Magic;
		std::uint32_t VersionMajor;
		std::uint32_t VersionMinor;
		std::uint32_t HeaderLength;
		std::uint32_t SegmentCount;
		std::uint32_t PathDataSize;
		Guid PDUID;
		std::uint32_t Flags;
		std::uint8_t Reserved0[0x38];
	};

#pragma pack(pop)

	enum struct XvdSegmentMetadataSegmentFlags : std::uint16_t {

		KeepEncryptedOnDisk = 1
	};

	constexpr auto operator|(XvdSegmentMetadataSegmentFlags left, XvdSegmentMetadataSegmentFlags right) noexcept -> XvdSegmentMetadataSegmentFlags {

		return static_cast<XvdSegmentMetadataSegmentFlags>(std::to_underlying(left) | std::to_underlying(right));
	}

	constexpr auto operator&(XvdSegmentMetadataSegmentFlags left, XvdSegmentMetadataSegmentFlags right) noexcept -> XvdSegmentMetadataSegmentFlags {

		return static_cast<XvdSegmentMetadataSegmentFlags>(std::to_underlying(left) & std::to_underlying(right));
	}

#pragma pack(push, 1)

	struct XvdSegmentMetadataSegment {

		XvdSegmentMetadataSegmentFlags Flags;
		std::uint16_t PathLength;
		std::uint32_t PathOffset;
		std::uint64_t FileSize;
	};

#pragma pack(pop)
}