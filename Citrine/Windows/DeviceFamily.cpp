#include "pch.h"
#include "DeviceFamily.h"

#include "Core/Util/ParseInteger.h"

#include <winrt/Windows.System.Profile.h>

namespace winrt {

	using namespace Windows::System::Profile;
}

namespace Citrine::Windows {

	auto DeviceFamilyVersion::FromInteger(std::uint64_t versionNumber) noexcept -> DeviceFamilyVersion {

		return {

			static_cast<std::uint16_t>((versionNumber & 0xFFFF000000000000) >> 48),
			static_cast<std::uint16_t>((versionNumber & 0x0000FFFF00000000L) >> 32),
			static_cast<std::uint16_t>((versionNumber & 0x00000000FFFF0000) >> 16),
			static_cast<std::uint16_t>((versionNumber & 0x000000000000FFFF))
		};
	}

	auto DeviceFamilyVersion::ToInteger(this DeviceFamilyVersion version) noexcept -> std::uint64_t {

		return {

			(std::uint64_t{ version.Major } << 48) |
			(std::uint64_t{ version.Minor } << 32) |
			(std::uint64_t{ version.Build } << 16) |
			(std::uint64_t{ version.Revision })
		};
	}

	auto DeviceFamilyInfo::Get() -> DeviceFamilyInfo const& {

		static auto info = [] static {

			auto versionInfo = winrt::AnalyticsInfo::VersionInfo();
			auto deviceForm = winrt::AnalyticsInfo::DeviceForm();

			return DeviceFamilyInfo{

				.DeviceFamily = winrt::to_string(versionInfo.DeviceFamily()),
				.DeviceFamilyVersion = DeviceFamilyVersion::FromInteger(ParseInteger<std::uint64_t>(versionInfo.DeviceFamilyVersion()).value_or({})),
				.DeviceForm = winrt::to_string(deviceForm)
			};
		}();
		return info;
	}
}