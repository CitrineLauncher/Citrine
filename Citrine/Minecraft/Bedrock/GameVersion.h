#pragma once

#include "Core/Util/VersionNumber.h"
#include "Windows/AppModel.h"

namespace Citrine::Minecraft::Bedrock {

    struct GameVersion : VersionNumberBase<GameVersion, 4> {

        static auto FromWindowsAppPackageVersion(Windows::PackageVersion packageVersion) noexcept -> GameVersion;

        constexpr GameVersion() noexcept = default;

        constexpr GameVersion(std::uint16_t baseline, std::uint16_t major, std::uint16_t minor, std::uint16_t revision) noexcept

            : Baseline(baseline)
            , Major(major)
            , Minor(minor)
            , Revision(revision)
        {}

        constexpr GameVersion(GameVersion const&) noexcept = default;
        constexpr auto operator=(GameVersion const&) noexcept -> GameVersion& = default;

        constexpr auto operator<=>(GameVersion const&) const noexcept -> std::strong_ordering = default;

        std::uint16_t Baseline{};
        std::uint16_t Major{};
        std::uint16_t Minor{};
        std::uint16_t Revision{};
    };
}
