#pragma once

#include "VersionNumber.h"

#include <cstdint>

namespace Citrine {

	struct SemanticVersion : VersionNumberBase<SemanticVersion, 3> {

        constexpr SemanticVersion() noexcept = default;

        constexpr SemanticVersion(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) noexcept

            : Major(major)
            , Minor(minor)
            , Patch(patch)
        {}

        constexpr SemanticVersion(SemanticVersion const&) noexcept = default;
        constexpr auto operator=(SemanticVersion const&) noexcept -> SemanticVersion& = default;

        constexpr auto operator<=>(SemanticVersion const&) const noexcept -> std::strong_ordering = default;

		std::uint32_t Major{};
		std::uint32_t Minor{};
		std::uint32_t Patch{};
	};
}