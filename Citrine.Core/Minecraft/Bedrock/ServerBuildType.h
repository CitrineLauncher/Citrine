#pragma once

namespace Citrine::Minecraft::Bedrock {

	enum struct ServerBuildType : std::uint8_t {

		Unknown,
		Release,
		Preview
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::ServerBuildType> {

		using enum ::Citrine::Minecraft::Bedrock::ServerBuildType;

		static constexpr auto value = enumerate(
			"Release", Release,
			"Preview", Preview
		);
	};
}