#pragma once

#include "Core/Util/Concepts.h"

#include <format>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	enum struct GamePlatform : std::uint8_t {

		Unknown,
		WindowsUWP,
		WindowsGDK,
		XboxUWP
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePlatform> {

		using enum ::Citrine::Minecraft::Bedrock::GamePlatform;

		static constexpr auto value = enumerate(
			"WindowsUWP", WindowsUWP,
			"WindowsGDK", WindowsGDK,
			"XboxUWP", XboxUWP
		);
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::GamePlatform, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Minecraft::Bedrock::GamePlatform platform, auto& ctx) const -> auto {

			using enum ::Citrine::Minecraft::Bedrock::GamePlatform;

			auto str = std::string_view{ "<unknown>" };
			switch (platform) {
			case WindowsUWP:	str = "WindowsUWP";	break;
			case WindowsGDK:	str = "WindowsGDK";	break;
			case XboxUWP:		str = "XboxUWP";	break;
			}

			return std::ranges::copy(str, ctx.out()).out;
		}
	};
}