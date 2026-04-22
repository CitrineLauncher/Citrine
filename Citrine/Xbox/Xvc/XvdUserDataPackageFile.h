#pragma once

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct alignas(4) XvdUserDataPackageFile {

		wchar_t FilePath[260];
		std::uint32_t FileSize;
		std::uint32_t Offset;
	};

#pragma pack(pop)
}