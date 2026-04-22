#pragma once

#include "ZipFlags.h"
#include "ZipCompressionMethod.h"

#include <cstdint>
#include <bit>

namespace Citrine {

#pragma pack(push, 1)

	struct ZipLocalFileHeader {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0304);

		std::uint32_t Signature;
		std::uint16_t MinVersion;
		ZipFlags Flags;
		ZipCompressionMethod CompressionMethod;
		std::uint16_t LastModificationTime;
		std::uint16_t LastModificationDate;
		std::uint32_t Crc32;
		std::uint32_t CompressedSize;
		std::uint32_t UncompressedSize;
		std::uint16_t FileNameLength;
		std::uint16_t ExtraFieldLength;
	};

#pragma pack(pop)
}