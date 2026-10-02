#pragma once

namespace Citrine::Minecraft::Bedrock {

	enum struct ServerPlatform : std::uint8_t {

		Unknown,
		Windows,
		Linux
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::ServerPlatform> {

		using enum ::Citrine::Minecraft::Bedrock::ServerPlatform;

		static constexpr auto value = enumerate(
			"Windows", Windows,
			"Linux", Linux
		);
	};
}