#pragma once

#include "GameVersion.h"
#include "ServerBuildType.h"
#include "ServerPlatform.h"
#include "PackageCollection.h"

#include "Core/Net/Url.h"
#include "Windows/AppModel.h"

#include <string>
#include <filesystem>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	struct ServerPackageInfo {

		GameVersion Version;
		ServerBuildType BuildType{};
		ServerPlatform Platform{};
		std::filesystem::path Path;
	};

	struct ServerPackageInfoQuery {

		GameVersion Version;
		ServerBuildType BuildType{};
		ServerPlatform Platform{};
	};

	struct ServerPackageInfoEqualityComparer {

		using is_transparent = void;

		auto operator()(auto const& left, auto const& right) const -> bool {

			return
				left.Version == right.Version &&
				left.BuildType == right.BuildType &&
				left.Platform == right.Platform;
		}
	};

	using ServerPackageInfoCollection = PackageCollection<ServerPackageInfo, ServerPackageInfoEqualityComparer>;

	struct ServerPackageMeta {

		using BaseUrlT = Url;
		using PackagesT = ServerPackageInfoCollection;

		BaseUrlT BaseUrl;
		PackagesT Packages;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::ServerPackageInfo> {

		using T = ::Citrine::Minecraft::Bedrock::ServerPackageInfo;

		static constexpr auto value = object(
			"Version", &T::Version,
			"BuildType", &T::BuildType,
			"Platform", &T::Platform
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::ServerPackageMeta> {

		using T = ::Citrine::Minecraft::Bedrock::ServerPackageMeta;

		static constexpr auto value = object(
			"BaseUrl", &T::BaseUrl,
			"Packages", &T::Packages
		);
	};
}