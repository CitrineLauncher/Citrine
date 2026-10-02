#pragma once

#include "UpdateInfo.g.h"

#include "ReleaseInfo.h"

namespace winrt::Citrine::implementation
{
    struct UpdateInfo : UpdateInfoT<UpdateInfo>
    {
        UpdateInfo(::Citrine::ReleaseInfo releaseInfo, winrt::Windows::Foundation::Uri releaseUrl);

        auto Version() const noexcept -> winrt::hstring;
        auto ReleaseChannel() const noexcept -> winrt::hstring;
        auto ReleaseTag() const noexcept -> winrt::hstring;
        auto ReleaseUrl() const noexcept -> winrt::Windows::Foundation::Uri;

        auto ReleaseInfo() const noexcept -> ::Citrine::ReleaseInfo const&;

    private:

        ::Citrine::ReleaseInfo releaseInfo;
        winrt::hstring version;
        winrt::hstring releaseTag;
        winrt::Windows::Foundation::Uri releaseUrl;
    };
}

namespace winrt::Citrine::factory_implementation
{
    /*
    struct UpdateInfo : UpdateInfoT<UpdateInfo, implementation::UpdateInfo>
    {
    };
    */  
}
