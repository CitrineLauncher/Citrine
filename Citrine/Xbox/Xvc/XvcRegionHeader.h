#pragma once

#include "XvcRegionId.h"

#include <cstdint>
#include <utility>

namespace Citrine::Xbox {

	enum struct XvcRegionFlags : std::uint32_t {

		SystemMetadata = 0x00000001,
		Preview = 0x00000002,
		InitialPlay = 0x00000004,
		OnDemand = 0x00000008,
		Unencrypted = 0x00000010
	};

	constexpr auto operator|(XvcRegionFlags left, XvcRegionFlags right) noexcept -> XvcRegionFlags {

		return XvcRegionFlags{ std::to_underlying(left) | std::to_underlying(right) };
	}

	constexpr auto operator&(XvcRegionFlags left, XvcRegionFlags right) noexcept -> XvcRegionFlags {

		return XvcRegionFlags{ std::to_underlying(left) & std::to_underlying(right) };
	}

#pragma pack(push, 1)

	struct alignas(4) XvcRegionHeader {

		XvcRegionId Id;
		std::uint16_t KeyIndex;
		std::uint16_t Spare0;
		XvcRegionFlags Flags;
		std::uint32_t FirstSegmentIndex;
		wchar_t Description[0x20];
		std::uint64_t Offset;
		std::uint64_t Length;
		std::uint64_t Hash;
		std::uint8_t Reserved0[0x18];
	};

#pragma pack(pop)
}