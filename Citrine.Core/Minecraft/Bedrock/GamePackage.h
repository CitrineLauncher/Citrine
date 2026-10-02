#pragma once

#include "GameVersion.h"
#include "GameBuildType.h"
#include "GamePlatform.h"
#include "GamePackageMeta.h"
#include "PackageCollection.h"

#include "Core/Util/Concepts.h"
#include "Core/Util/StringLiteral.h"
#include "Core/Util/JsonWrappers.h"
#include "Windows/AppModel.h"

#include <string>
#include <filesystem>
#include <tuple>
#include <format>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	enum struct GamePackageOrigin : std::uint8_t {

		Unknown,
		Meta,
		Import
	};

	struct GamePackageIdentity {

		GameVersion Version;
		GameBuildType BuildType{};
		GamePlatform Platform{};
		Windows::PackageArchitecture Architecture{};
		GamePackageOrigin Origin{};
		std::uint16_t InstallId{};
	};

	struct GamePackage : GamePackageIdentity {

		std::filesystem::path InstallLocation;
		mutable std::string NameTag;
	};

	struct GamePackageEqualityComparer {

		using is_transparent = void;

		auto operator()(auto const& left, auto const& right) const -> bool {

			return
				left.Version == right.Version &&
				left.BuildType == right.BuildType &&
				left.Platform == right.Platform &&
				left.Architecture == right.Architecture &&
				left.Origin == right.Origin &&
				left.InstallId == right.InstallId;
		}
	};

	using GamePackageCollection = PackageCollection<GamePackage, GamePackageEqualityComparer>;
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageOrigin> {

		using enum ::Citrine::Minecraft::Bedrock::GamePackageOrigin;

		static constexpr auto value = enumerate(
			"Meta", Meta,
			"Import", Import
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageIdentity> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageIdentity;

		static constexpr auto value = object(
			"Version", &T::Version,
			"BuildType", &T::BuildType,
			"Platform", &T::Platform,
			"Architecture", &T::Architecture,
			"Origin", &T::Origin,
			"InstallId", &T::InstallId
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackage> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackage;

		static constexpr auto value = object(
			"Version", &T::Version,
			"BuildType", &T::BuildType,
			"Platform", &T::Platform,
			"Architecture", &T::Architecture,
			"Origin", &T::Origin,
			"InstallId", &T::InstallId,
			"InstallLocation", &T::InstallLocation,
			"NameTag", SkipDefault<&T::NameTag>
		);
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::GamePackageOrigin, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Minecraft::Bedrock::GamePackageOrigin origin, auto& ctx) const -> auto {

			using enum ::Citrine::Minecraft::Bedrock::GamePackageOrigin;

			auto str = std::string_view{ "<unknown>" };
			switch (origin) {
			case Meta:		str = "Meta";	break;
			case Import:	str = "Import";	break;
			}

			return std::ranges::copy(str, ctx.out()).out;
		}
	};

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::GamePackageIdentity, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Minecraft::Bedrock::GamePackageIdentity const& id, auto& ctx) const -> auto {

			return format_to(ctx.out(), GetFormatString(), id.Version, id.BuildType, id.Platform, id.Architecture, id.Origin, id.InstallId);
		}

	private:

		static consteval auto GetFormatString() noexcept -> auto const& {

			static constexpr auto fmt = ::Citrine::StringLiteralCast<CharT>(
				"(Version: {}, BuildType: {}, Platform: {}, Architecture: {}, Origin: {}, InstallId: {})"
			);
			return fmt;
		}
	};

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Minecraft::Bedrock::GamePackage, CharT> : formatter<::Citrine::Minecraft::Bedrock::GamePackageIdentity, CharT> {};
}