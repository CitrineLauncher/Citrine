#include "pch.h"
#include "DeviceFamily.h"

#include "Core/Util/ParseInteger.h"
#include "Core/Util/Ascii.h"
#include "Core/Util/Frozen.h"

#include <winrt/Windows.System.Profile.h>

namespace winrt {

	using namespace Windows::System::Profile;
}

namespace Citrine::Windows {

    DeviceFamily::DeviceFamily(std::string_view name) noexcept {

        struct NameComparer {

            static constexpr auto operator()(std::string_view left, std::string_view right) noexcept -> bool {

                constexpr auto toLower = [](char ch) static { return Ascii::ToLower(ch); };

                return std::ranges::lexicographical_compare(left, right, {}, toLower, toLower);
            }
        };

        static constexpr auto knownDeviceFamilies = MakeFrozenMap<std::string_view, DeviceFamily, NameComparer>({

			{ "Windows.Universal", Universal },
			{ "Windows.Windows8x", Windows8x },
			{ "Windows.WindowsPhone8x", WindowsPhone8x },
			{ "Windows.Desktop", Desktop },
			{ "Windows.Mobile", Mobile },
			{ "Windows.Xbox", Xbox },
			{ "Windows.Team", Team },
			{ "Windows.IoT", IoT },
			{ "Windows.IoTHeadless", IoTHeadless },
			{ "Windows.Server", Server },
			{ "Windows.Holographic", Holographic },
			{ "Windows.XBoxSRA", XBoxSRA },
			{ "Windows.XBoxERA", XBoxERA },
			{ "Windows.ServerNano", ServerNano },
			{ "Windows.8828080", _8828080 },
			{ "Windows.7067329", _7067329 },
			{ "Windows.Core", Core },
			{ "Windows.CoreHeadless", CoreHeadless }
        });

        auto it = knownDeviceFamilies.find(name);
        if (it != knownDeviceFamilies.end())
            value = it->second;
    }

    auto DeviceFamily::Name() const noexcept -> std::string_view {

        auto name = std::string_view{ "<unknown>" };
        switch (value) {
		case Universal:         name = "Windows.Universal";         break;
		case Windows8x:         name = "Windows.Windows8x";         break;
		case WindowsPhone8x:    name = "Windows.WindowsPhone8x";    break;
		case Desktop:           name = "Windows.Desktop";           break;
		case Mobile:            name = "Windows.Mobile";            break;
		case Xbox:              name = "Windows.Xbox";              break;
		case Team:              name = "Windows.Team";              break;
		case IoT:               name = "Windows.IoT";               break;
		case IoTHeadless:       name = "Windows.IoTHeadless";       break;
		case Server:            name = "Windows.Server";            break;
		case Holographic:       name = "Windows.Holographic";       break;
		case XBoxSRA:           name = "Windows.XBoxSRA";           break;
		case XBoxERA:           name = "Windows.XBoxERA";           break;
		case ServerNano:        name = "Windows.ServerNano";        break;
		case _8828080:          name = "Windows.8828080";           break;
		case _7067329:          name = "Windows.7067329";           break;
		case Core:              name = "Windows.Core";              break;
		case CoreHeadless:      name = "Windows.CoreHeadless";      break;
        }
        return name;
    }

    DeviceFamily::operator std::string() const {

        return std::string{ Name() };
    }

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

				.DeviceFamily = Windows::DeviceFamily{ winrt::to_string(versionInfo.DeviceFamily()) },
				.DeviceFamilyVersion = DeviceFamilyVersion::FromInteger(ParseInteger<std::uint64_t>(versionInfo.DeviceFamilyVersion()).value_or({})),
				.DeviceForm = winrt::to_string(deviceForm)
			};
		}();
		return info;
	}
}