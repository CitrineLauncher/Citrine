#pragma once

#include "Core/Util/VersionNumber.h"

#include <compare>

namespace Citrine::Windows {

	struct VersionInfo : VersionNumberBase<VersionInfo, 3> {

		constexpr VersionInfo() noexcept = default;

		constexpr VersionInfo(std::uint32_t major, std::uint32_t minor, std::uint32_t build) noexcept
		
			: Major(major)
			, Minor(minor)
			, Build(build)
		{}

		constexpr VersionInfo(VersionInfo const&) noexcept = default;
		constexpr auto operator=(VersionInfo const&) noexcept -> VersionInfo& = default;

		constexpr auto operator<=>(VersionInfo const&) const noexcept -> std::strong_ordering = default;

		std::uint32_t Major{};
		std::uint32_t Minor{};
		std::uint32_t Build{};
	};

	struct BuildInfo {

		static VersionInfo const Version;
	};
}
