#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Util/Event.h"
#include "Core/Storage/JsonStorage.h"

#include <winrt/Citrine.h>

#include <filesystem>

#include <glaze/json.hpp>

namespace Citrine {

	class MinecraftBedrockGameManagerSettings;

	class MinecraftBedrockGameManager {
	public:

		static auto InitializeAsync() -> void;
		static auto InitializationCompleted(EventHandler<> handler) -> EventToken;
		static auto InitializationCompleted(EventToken&& token) -> void;

		static auto Settings() -> MinecraftBedrockGameManagerSettings&;

		static auto ReleaseGamePackages() -> winrt::Citrine::IObservableCollectionView;
		static auto PreviewGamePackages() -> winrt::Citrine::IObservableCollectionView;
		static auto ImportedGamePackages() -> winrt::Citrine::IObservableCollectionView;

		static auto ValidateInstallLocation(std::filesystem::path const& path) -> winrt::Citrine::InstallLocationValidationResult;

		static auto InstallGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void;
		static auto InitiateGamePackageImportAsync(std::filesystem::path gamePackageLocation) -> Task<winrt::Citrine::MinecraftBedrockGamePackageImportContext>;
		static auto ImportGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageImportContext importContext, winrt::hstring nameTag) -> void;
		static auto LaunchGamePackageAsync(winrt::Citrine::MinecraftBedrockGameLaunchArgs launchArgs) -> Task<winrt::Citrine::MinecraftBedrockGameLaunchResult>;
		static auto RenameGamePackage(winrt::Citrine::MinecraftBedrockGamePackageItem const& item, winrt::hstring const& nameTag) -> void;
		static auto UninstallGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void;

		static auto GetGameDirectory(winrt::Citrine::MinecraftBedrockGamePackageItem const& item) -> std::filesystem::path;
		static auto GetGameDataDirectory(winrt::Citrine::MinecraftBedrockGamePackageItem const& item) -> std::filesystem::path;

		static auto PauseGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void;
		static auto ResumeGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void;
		static auto CancelGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void;

		static auto Shutdown() -> void;
	};

	class MinecraftBedrockGameManagerSettings {
	public:

		auto GameInstallLocation() const noexcept -> std::filesystem::path const&;
		auto GameInstallLocation(std::filesystem::path value) noexcept -> void;

		auto Save() -> StorageOperationResult;

	protected:

		constexpr MinecraftBedrockGameManagerSettings() = default;

		MinecraftBedrockGameManagerSettings(MinecraftBedrockGameManagerSettings const&) = delete;
		auto operator=(MinecraftBedrockGameManagerSettings const&) = delete;

		auto Open(std::filesystem::path const& path) -> bool;
		auto Close() -> void;

		auto Load() -> StorageOperationResult;

		struct SettingsData {

			std::filesystem::path GameInstallLocation;
		};

		friend struct ::glz::meta<SettingsData>;

		JsonStorage<SettingsData> storage;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::MinecraftBedrockGameManagerSettings::SettingsData> {

		using T = ::Citrine::MinecraftBedrockGameManagerSettings::SettingsData;

		static constexpr auto value = object(
			"GameInstallLocation", &T::GameInstallLocation
		);
	};
}