#pragma once

#include "Core/Util/SemanticVersion.h"

#include <vector>

#include <glaze/json.hpp>

namespace Citrine {

	enum struct ReleaseChannel {

		Unknown,
		Stable,
		Preview
	};

	struct ReleaseInfo {

		SemanticVersion Version;
		ReleaseChannel Channel{};
		std::string Tag;
	};

	using ReleaseInfoCollection = std::vector<ReleaseInfo>;
}

namespace glz {

	template<>
	struct meta<::Citrine::ReleaseChannel> {

		using enum ::Citrine::ReleaseChannel;

		static constexpr auto value = enumerate(
			"Stable", Stable,
			"Preview", Preview
		);
	};

	template<>
	struct meta<::Citrine::ReleaseInfo> {

		using T = ::Citrine::ReleaseInfo;

		static constexpr auto value = object(
			"Version", &T::Version,
			"Channel", &T::Channel,
			"Tag", &T::Tag
		);
	};
}