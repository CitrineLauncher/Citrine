#include "pch.h"
#include "UpdateInfo.h"
#if __has_include("UpdateInfo.g.cpp")
#include "UpdateInfo.g.cpp"
#endif

#include "Core/Util/TrivialArray.h"

using namespace Citrine;

namespace winrt {

    using namespace Windows::Foundation;
}

namespace winrt::Citrine::implementation
{
    UpdateInfo::UpdateInfo(::Citrine::ReleaseInfo releaseInfo, winrt::Windows::Foundation::Uri releaseUrl)

        : releaseInfo(std::move(releaseInfo))
        , releaseUrl(std::move(releaseUrl))
    {
        auto& versionNumber = this->releaseInfo.Version;
        auto buffer = TrivialArray<wchar_t, VersionNumberFormatter::MaxFormattedSize(versionNumber)>{};
        version = std::wstring_view{ buffer.data(), VersionNumberFormatter::FormatTo(buffer.data(), versionNumber) };

        releaseTag = winrt::to_hstring(this->releaseInfo.Tag);
    }

    auto UpdateInfo::Version() const noexcept -> winrt::hstring {

        return version;
    }

    auto UpdateInfo::ReleaseChannel() const noexcept -> winrt::hstring {

        struct Strings {

            winrt::hstring Stable = L"Stable";
            winrt::hstring Preview = L"Preview";
        };
        static auto const strings = Strings{};

        using enum ReleaseChannel;

        auto releaseChannel = winrt::hstring{};
        switch (releaseInfo.Channel) {
        case Stable:    releaseChannel = strings.Stable;    break;
        case Preview:   releaseChannel = strings.Preview;   break;
        }
        return releaseChannel;
    }

    auto UpdateInfo::ReleaseTag() const noexcept -> winrt::hstring {

        return releaseTag;
    }

    auto UpdateInfo::ReleaseUrl() const noexcept -> winrt::Uri {

        return releaseUrl;
    }

    auto UpdateInfo::ReleaseInfo() const noexcept -> ::Citrine::ReleaseInfo const& {

        return releaseInfo;
    }
}
