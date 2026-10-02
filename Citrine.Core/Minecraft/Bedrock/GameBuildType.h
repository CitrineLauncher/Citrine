#pragma once

#include "Core/Util/Concepts.h"

#include <format>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	enum struct GameBuildType : std::uint8_t {

		Unknown,
		Release,
		Preview,
		Beta
	};

	constexpr auto operator<=>(GameBuildType left, GameBuildType right) noexcept -> std::strong_ordering {

		return (std::uint8_t{} - std::to_underlying(left)) <=> (std::uint8_t{} - std::to_underlying(right));
	}
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GameBuildType> {

		using enum ::Citrine::Minecraft::Bedrock::GameBuildType;

		static constexpr auto value = enumerate(
			"Release", Release,
			"Preview", Preview,
			"Beta", Beta
		);
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::GameBuildType, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Minecraft::Bedrock::GameBuildType buildType, auto& ctx) const -> auto {

			using enum ::Citrine::Minecraft::Bedrock::GameBuildType;

			auto str = std::string_view{ "<unknown>" };
			switch (buildType) {
			case Release:	str = "Release";	break;
			case Preview:	str = "Preview";	break;
			case Beta:		str = "Beta";		break;
			}

			return std::ranges::copy(str, ctx.out()).out;
		}
	};
}