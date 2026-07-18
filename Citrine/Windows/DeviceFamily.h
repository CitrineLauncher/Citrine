#pragma once

#include "Core/Util/VersionNumber.h"

namespace Citrine::Windows {

    struct DeviceFamilyVersion : VersionNumberBase<DeviceFamilyVersion, 4> {

        constexpr DeviceFamilyVersion() noexcept = default;

        constexpr DeviceFamilyVersion(std::uint16_t major, std::uint16_t minor, std::uint16_t build, std::uint16_t revision) noexcept

            : Major(major)
            , Minor(minor)
            , Build(build)
            , Revision(revision)
        {}

        constexpr DeviceFamilyVersion(DeviceFamilyVersion const&) noexcept = default;
        constexpr auto operator=(DeviceFamilyVersion const&) noexcept -> DeviceFamilyVersion& = default;

        constexpr auto operator<=>(DeviceFamilyVersion const&) const noexcept -> std::strong_ordering = default;

        std::uint16_t Major{};
        std::uint16_t Minor{};
        std::uint16_t Build{};
        std::uint16_t Revision{};
    };
}