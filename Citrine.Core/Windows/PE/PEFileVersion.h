#pragma once

#include "Core/Util/VersionNumber.h"
#include "Core/IO/Buffer.h"

#include <filesystem>

namespace Citrine::Windows {

	struct PEFileVersion : VersionNumberBase<PEFileVersion, 4> {

        constexpr PEFileVersion() noexcept = default;

        constexpr PEFileVersion(std::uint16_t major, std::uint16_t minor, std::uint16_t build, std::uint16_t revision) noexcept

            : Major(major)
            , Minor(minor)
            , Build(build)
            , Revision(revision)
        {}

        constexpr PEFileVersion(PEFileVersion const&) noexcept = default;
        constexpr auto operator=(PEFileVersion const&) noexcept -> PEFileVersion& = default;

        constexpr auto operator<=>(PEFileVersion const&) const noexcept -> std::strong_ordering = default;

        std::uint16_t Major{};
        std::uint16_t Minor{};
        std::uint16_t Build{};
        std::uint16_t Revision{};
	};

    auto GetPEFileVersion(std::filesystem::path const& path, PEFileVersion& version, BasicBuffer* buffer = nullptr) -> bool;
}
