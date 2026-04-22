#pragma once

#include <cstdint>

namespace Citrine::Xbox {

	enum struct XvdUserDataType : std::uint32_t {

		PackageFiles
	};

#pragma pack(push, 1)

	struct alignas(4) XvdUserDataHeader {

		std::uint32_t HeaderLength;
		std::uint32_t HeaderVersion;
		XvdUserDataType DataType;
		std::uint32_t DataLength;
	};

#pragma pack(pop)
}