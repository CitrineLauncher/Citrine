#pragma once

#include <cstdint>
#include <bit>

namespace Citrine {

#pragma pack(push, 1)

	struct ZipDataDescriptor {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0708);

		std::uint32_t Signature;
		std::uint32_t Crc32;
		std::uint32_t CompressedSize;
		std::uint32_t UncompressedSize;
	};

	struct Zip64DataDescriptor {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0708);

		std::uint32_t Signature;
		std::uint32_t Crc32;
		std::uint64_t CompressedSize;
		std::uint64_t UncompressedSize;
	};

#pragma pack(pop)
}