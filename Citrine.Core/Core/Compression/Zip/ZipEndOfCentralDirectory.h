#pragma once

#include <cstdint>
#include <bit>

namespace Citrine {

#pragma pack(push, 1)

	struct ZipEndOfCentralDirectory {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0506);

		std::uint32_t Signature;
		std::uint16_t DiskNumber;
		std::uint16_t StartingDiskNumber;
		std::uint16_t DiskEntryCount;
		std::uint16_t TotalEntryCount;
		std::uint32_t CentralDirectorySize;
		std::uint32_t CentralDirectoryOffset;
		std::uint16_t CommentLength;
	};

	struct Zip64EndOfCentralDirectory {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0606);

		std::uint32_t Signature;
		std::uint64_t Size;
		std::uint16_t VersionMadeBy;
		std::uint16_t MinVersion;
		std::uint32_t DiskNumber;
		std::uint32_t StartingDiskNumber;
		std::uint64_t DiskEntryCount;
		std::uint64_t TotalEntryCount;
		std::uint64_t CentralDirectorySize;
		std::uint64_t CentralDirectoryOffset;
	};

	struct Zip64EndOfCentralDirectoryLocator {

		static constexpr auto ExpectedSignature = std::byteswap<std::uint32_t>(0x504b0607);

		std::uint32_t Signature;
		std::uint32_t StartingDiskNumber;
		std::uint64_t Offset;
		std::uint32_t TotalDiskCount;
	};

#pragma pack(pop)
}