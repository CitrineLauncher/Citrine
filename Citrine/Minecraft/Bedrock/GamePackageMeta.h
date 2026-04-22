#pragma once

#include "GameVersion.h"
#include "GameBuildType.h"
#include "GamePlatform.h"
#include "PackageCollection.h"

#include "Core/Util/JsonWrappers.h"
#include "Core/Net/Url.h"
#include "Windows/AppModel.h"

#include <string>
#include <filesystem>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	struct GamePackageInfo {

		GameVersion Version;
		GameBuildType BuildType{};
		GamePlatform Platform{};
		Windows::PackageArchitecture Architecture{};
		std::string UpdateId;
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

		operator bool() const noexcept {

			return Score > 0;
		}

		auto operator<=>(GamePackageCompatibility const&) const noexcept -> std::strong_ordering = default;

		std::uint8_t Score{};
	};

	auto CheckGamePackageCompatibility(GameVersion version, GamePlatform platform, Windows::PackageArchitecture architecture) noexcept -> GamePackageCompatibility;

	auto CheckGamePackageCompatibility(auto const& package) noexcept -> GamePackageCompatibility {

		return CheckGamePackageCompatibility(package.Version, package.Platform, package.Architecture);
	}

	struct GamePackageDependencyInfo {

		std::string PackageFamilyName;
		Windows::PackageVersion Version;
		Windows::PackageArchitecture Architecture{};
		std::string UpdateId;
	};

	struct GamePackageMeta {

		struct BaseUrlsT {

			std::vector<Url> WindowsUWP;
			std::vector<Url> WindowsGDK;
		};

		using PackagesT = GamePackageInfoCollection;

		struct DependenciesT {

			std::vector<GamePackageDependencyInfo> WindowsUWP;
			std::vector<GamePackageDependencyInfo> WindowsGDK;
		};

		BaseUrlsT BaseUrls;
		PackagesT Packages;
		DependenciesT Dependencies;
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
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageDependencyInfo> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageDependencyInfo;

		static constexpr auto value = object(
			"PackageFamilyName", &T::PackageFamilyName,
			"Version", &T::Version,
			"Architecture", &T::Architecture,
			"UpdateId", &T::UpdateId
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
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageMeta::DependenciesT> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageMeta::DependenciesT;

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
			"Dependencies", &T::Dependencies
		);
	};
}