#pragma once

#include <cstdint>

namespace Citrine::Xbox {

#pragma pack(push, 1)

	struct alignas(4) XvdUserDataPackageFilesHeader {

		std::uint32_t Version;
		wchar_t PackageFullName[260];
		std::uint32_t EntryCount;
	};

#pragma pack(pop)
}