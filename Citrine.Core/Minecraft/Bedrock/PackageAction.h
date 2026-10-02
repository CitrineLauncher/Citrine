#pragma once

#include "Core/Util/Concepts.h"

#include <format>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	enum struct PackageAction : std::uint8_t {

		None,
		Install,
		Import,
		Repair,
		Register,
		Launch,
		Unregister,
		Uninstall
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::PackageAction> {

		using enum ::Citrine::Minecraft::Bedrock::PackageAction;

		static constexpr auto value = enumerate(
			"None", None,
			"Install", Install,
			"Import", Import,
			"Repair", Repair,
			"Register", Register,
			"Launch", Launch,
			"Unregister", Unregister,
			"Uninstall", Uninstall
		);
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::PackageAction, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Minecraft::Bedrock::PackageAction buildType, auto& ctx) const -> auto {

			using enum ::Citrine::Minecraft::Bedrock::PackageAction;

			auto str = std::string_view{ "<unknown>" };
			switch (buildType) {
			case None:			str = "None";		break;
			case Install:		str = "Install";	break;
			case Import:		str = "Import";		break;
			case Repair:		str = "Repair";		break;
			case Register:		str = "Register";	break;
			case Launch:		str = "Launch";		break;
			case Unregister:	str = "Unregister";	break;
			case Uninstall:		str = "Uninstall";	break;
			}

			return std::ranges::copy(str, ctx.out()).out;
		}
	};
}