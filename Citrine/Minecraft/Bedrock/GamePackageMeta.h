#pragma once

#include "GameVersion.h"
#include "GameBuildType.h"
#include "GamePlatform.h"
#include "PackageCollection.h"

#include "Core/Util/JsonWrappers.h"
#include "Core/Net/Url.h"
#include "Core/Util/Guid.h"
#include "Windows/AppModel.h"
#include "Xbox/PackageModel.h"

#include <string>
#include <filesystem>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	struct GamePackageInfo {

		using UpdateIdVariantT = std::variant<std::monostate, Guid, Xbox::PackageVersionIdentifier>;

		GameVersion Version;
		GameBuildType BuildType{};
		GamePlatform Platform{};
		Windows::PackageArchitecture Architecture{};
		UpdateIdVariantT UpdateId;
		std::filesystem::path Path;
	};

	struct GamePackageInfoQuery {

		GameVersion Version;
		GameBuildType BuildType{};
		GamePlatform Platform{};
		Windows::PackageArchitecture Architecture{};
	};

	struct GamePackageInfoEqualityComparer {

		using is_transparent = void;

		auto operator()(auto const& left, auto const& right) const -> bool {

			return
				left.Version == right.Version &&
				left.BuildType == right.BuildType &&
				left.Platform == right.Platform &&
				left.Architecture == right.Architecture;
		}
	};

	using GamePackageInfoCollection = PackageCollection<GamePackageInfo, GamePackageInfoEqualityComparer>;

	struct GamePackageCompatibility {

		explicit operator bool() const noexcept {

			return Score > 0;
		}

		auto operator<=>(GamePackageCompatibility const&) const noexcept -> std::strong_ordering = default;

		std::uint8_t Score{};
	};

	auto CheckGamePackageCompatibility(GameVersion version, GamePlatform platform, Windows::PackageArchitecture architecture) noexcept -> GamePackageCompatibility;

	auto CheckGamePackageCompatibility(auto const& package) noexcept -> GamePackageCompatibility {

		return CheckGamePackageCompatibility(package.Version, package.Platform, package.Architecture);
	}

	struct GamePlatformDependencyInfo {

		std::string PackageFamilyName;
		Windows::PackageVersion Version;
	};

	struct GamePackageMeta {

		struct BaseUrlsT {

			std::vector<Url> WindowsUWP;
			std::vector<Url> WindowsGDK;
		};

		using PackagesT = GamePackageInfoCollection;

		struct PlatformDependenciesT {

			std::vector<GamePlatformDependencyInfo> WindowsUWP;
			std::vector<GamePlatformDependencyInfo> WindowsGDK;
		};

		BaseUrlsT BaseUrls;
		PackagesT Packages;
		PlatformDependenciesT PlatformDependencies;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageInfo> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageInfo;

		static constexpr auto value = object(
			"Version", &T::Version,
			"BuildType", &T::BuildType,
			"Platform", &T::Platform,
			"Architecture", &T::Architecture,
			"UpdateId", SkipDefault<&T::UpdateId>,
			"Path", SkipDefault<&T::Path>
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePlatformDependencyInfo> {

		using T = ::Citrine::Minecraft::Bedrock::GamePlatformDependencyInfo;

		static constexpr auto value = object(
			"PackageFamilyName", &T::PackageFamilyName,
			"Version", &T::Version
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageMeta::BaseUrlsT> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageMeta::BaseUrlsT;

		static constexpr auto value = object(
			"WindowsUWP", SkipDefault<&T::WindowsUWP>,
			"WindowsGDK", SkipDefault<&T::WindowsGDK>
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageMeta::PlatformDependenciesT> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageMeta::PlatformDependenciesT;

		static constexpr auto value = object(
			"WindowsUWP", SkipDefault<&T::WindowsUWP>,
			"WindowsGDK", SkipDefault<&T::WindowsGDK>
		);
	};

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageMeta> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageMeta;

		static constexpr auto value = object(
			"BaseUrls", &T::BaseUrls,
			"Packages", &T::Packages,
			"PlatformDependencies", &T::PlatformDependencies
		);
	};
}