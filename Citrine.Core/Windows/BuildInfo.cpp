#include "pch.h"
#include "BuildInfo.h"

#pragma comment(lib, "ntdll.lib")
extern "C" {

	typedef LONG NTSTATUS, * PNTSTATUS;

	NTSYSAPI auto NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation) -> ::NTSTATUS;
};

namespace Citrine::Windows {

	VersionInfo const BuildInfo::Version = [] -> VersionInfo {

		auto versionInfo = ::OSVERSIONINFOW{};
		versionInfo.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
		::RtlGetVersion(&versionInfo);

		return { versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber };
	}();
}