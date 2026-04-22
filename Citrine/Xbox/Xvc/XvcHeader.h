#pragma once

#include "Core/Util/Guid.h"

#include <cstdint>
#include <utility>

namespace Citrine::Xbox {

	enum struct XvcHeaderFlags : std::uint32_t {

		XtsOffsetsCached = 0x00000001,
		StrictRegionMatch = 0x00000002,
		SubFileUpdateRequested = 0x00000004
	};

	constexpr auto operator|(XvcHeaderFlags left, XvcHeaderFlags right) noexcept -> XvcHeaderFlags {

		return XvcHeaderFlags{ std::to_underlying(left) | std::to_underlying(right) };
	}

	constexpr auto operator&(XvcHeaderFlags left, XvcHeaderFlags right) noexcept -> XvcHeaderFlags {

		return XvcHeaderFlags{ std::to_underlying(left) & std::to_underlying(right) };
	}

#pragma pack(push, 1)

	struct alignas(4) XvcHeader {

		Guid ContentId;
		Guid KeyIds[0x40];
		std::uint8_t Reserved0[0x800];
		wchar_t Description[0x80];
		std::uint32_t Version;
		std::uint32_t RegionCount;
		XvcHeaderFlags Flags;
		std::uint16_t LanguageId;
		std::uint16_t KeyIdCount;
		std::uint32_t Type;
		std::uint32_t InitialPlayRegionId;
		std::uint64_t InitialPlayOffset;
		std::uint64_t CreationTime;
		std::uint32_t PreviewRegionId;
		std::uint32_t SegmentCount;
		std::uint64_t PreviewOffset;
		std::uint8_t UnusedSpace0[0x8];
		std::uint32_t RegionSpecifierCount;
		std::uint32_t XtsEntryCount;
		std::uint8_t Reserved1[0x50];
	};

#pragma pack(pop)
}