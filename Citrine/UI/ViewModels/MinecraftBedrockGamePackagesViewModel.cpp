#include "pch.h"
#include "MinecraftBedrockGamePackagesViewModel.h"
#if __has_include("MinecraftBedrockGamePackagesViewModel.g.cpp")
#include "MinecraftBedrockGamePackagesViewModel.g.cpp"
#endif

#include "Models/MinecraftBedrockGamePackageItem.h"
#include "Collections/FilterableCollectionView.h"

#include "ApplicationData.h"
#include "Locale/Localizer.h"
#include "Services/MinecraftBedrockGameManager.h"
#include "Services/ToastNotificationService.h"
#include "Windows/Shell.h"

#include <optional>

using namespace Citrine;
using namespace Minecraft::Bedrock;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Foundation::Collections;
	using namespace Microsoft::UI::Xaml::Data;
}

namespace {

	using GamePackageItem = winrt::Citrine::MinecraftBedrockGamePackageItem;
	using GamePackageItemImpl = winrt::Citrine::implementation::MinecraftBedrockGamePackageItem;
	using GamePackageStatus = winrt::Citrine::MinecraftBedrockGamePackageStatus;

	class GamePackageFilterProperties {
	public:

		GamePackageFilterProperties(GamePackageItem const& item) noexcept 

			: itemImpl(*winrt::get_self<GamePackageItemImpl>(item))
		{}

		static constexpr auto size() noexcept -> std::size_t {

			return 4;
		}

		template<std::size_t I> requires (I < size())
		auto get() const -> decltype(auto) {

			if constexpr (I == 0) {			// Version

				return itemImpl.Version();
			}
			else if constexpr (I == 1) {	// BuildType

				auto const& strings = stringResources->BuildType;
				using enum GameBuildType;

				auto buildType = std::wstring_view{};
				switch (itemImpl.Id().BuildType) {
				case Release:   buildType = strings.Release;    break;
				case Preview:   buildType = strings.Preview;    break;
				case Beta:      buildType = strings.Beta;       break;
				}
				return buildType;
			}
			else if constexpr (I == 2) {	// Platform

				auto const& strings = stringResources->Platform;
				using enum GamePlatform;

				auto platform = std::wstring_view{};
				switch (itemImpl.Id().Platform) {
				case XboxUWP:		[[fallthrough]];
				case WindowsUWP:	platform = strings.UWP;	break;
				case WindowsGDK:    platform = strings.GDK;	break;
				}
				return platform;
			}
			else if constexpr (I == 3) {	// NameTag

				return itemImpl.NameTag();
			}
		}

		static auto EnsureResources() -> void {

			if (!stringResources)
				stringResources.emplace();
		}

	private:

		struct StringResources {

			struct BuildTypeT {

				winrt::hstring Release = Localizer::GetString(L"BuildType_Release");
				winrt::hstring Preview = Localizer::GetString(L"BuildType_Preview");
				winrt::hstring Beta = Localizer::GetString(L"BuildType_Beta");
			};

			struct PlatformT {

				winrt::hstring UWP = Localizer::GetString(L"Platform_UWP");
				winrt::hstring GDK = Localizer::GetString(L"Platform_GDK");
			};

			BuildTypeT BuildType;
			PlatformT Platform;
		};
		static inline auto stringResources = std::optional<StringResources>{};

		GamePackageItemImpl const& itemImpl;
	};

	auto MakeFilterableGamePackageCollectionView(winrt::Citrine::IObservableCollectionView const& gamePackages) {

		struct ItemProjection {

			static auto operator()(GamePackageItem const& item) noexcept -> GamePackageFilterProperties {

				return GamePackageFilterProperties{ item };
			}
		};

		GamePackageFilterProperties::EnsureResources();
		return MakeFilterableCollectionView<GamePackageItem, ItemProjection>(gamePackages);
	}
}

namespace winrt::Citrine::implementation
{
	MinecraftBedrockGamePackagesViewModel::MinecraftBedrockGamePackagesViewModel(winrt::hstring packageSourceId)
	
		: packageSourceId(std::move(packageSourceId))
	{
		auto& sourceId = this->packageSourceId;

		if (sourceId == L"Releases") {

			gamePackages = MinecraftBedrockGameManager::ReleaseGamePackages();
		}
		else if (sourceId == L"Previews") {

			gamePackages = MinecraftBedrockGameManager::PreviewGamePackages();
		}
		else if (sourceId == L"Imports") {

			gamePackages = MinecraftBedrockGameManager::ImportedGamePackages();
		}
		else {

			throw winrt::hresult_invalid_argument{};
		}
		filteredGamePackages = MakeFilterableGamePackageCollectionView(gamePackages);

		gameManagerInitializationCompletedRevoker = MinecraftBedrockGameManager::InitializationCompleted([this] {

			using enum GamePackageStatus;

			if (SupportsImporting()) {

				canStartImporting = true;
				OnPropertyChanged(L"CanStartImporting");
			}
			else {

				for (auto const& gamePackage : gamePackages.as<winrt::IVectorView<GamePackageItem>>()) {

					auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
					auto status = gamePackageImpl->Status();

					if (status != NotInstalled && status != Installed)
						RegisterGamePackageStatusListener(gamePackage);
				}
			}
		});

		if (SupportsImporting())
			return;

		if (!installedGamePackagesFilter) {

			installedGamePackagesFilter = MakeItemFilter<GamePackageItem>([](GamePackageItem const& item) {

				auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
				return itemImpl->Status() != GamePackageStatus::NotInstalled;
			});
		}

		auto& settings = ApplicationData::LocalSettings();
		viewModeChangedRevoker = settings.PackageViewModeChanged([this](PackageViewMode value) {

			filteredGamePackages.SecondaryFilter(value == PackageViewMode::Installed
				? installedGamePackagesFilter
				: EmptyItemFilter<GamePackageItem>()
			);
		});

		if (settings.PackageViewMode() == PackageViewMode::Installed) {

			filteredGamePackages.SecondaryFilter(installedGamePackagesFilter);
		}
	}

	auto MinecraftBedrockGamePackagesViewModel::GamePackages() const noexcept -> Citrine::IObservableCollectionView {

		return gamePackages;
	}

	auto MinecraftBedrockGamePackagesViewModel::FilteredGamePackages() const noexcept -> Citrine::IFilterableCollectionView {

		return filteredGamePackages;
	}

	auto MinecraftBedrockGamePackagesViewModel::CurrentInstallLocation() const noexcept -> winrt::hstring {

		auto& settings = MinecraftBedrockGameManager::Settings();
		return winrt::hstring{ settings.GameInstallLocation().native() };
	}

	auto MinecraftBedrockGamePackagesViewModel::ValidateInstallLocation(winrt::hstring const& path) const noexcept -> Citrine::InstallLocationValidationResult {

		return MinecraftBedrockGameManager::ValidateInstallLocation(std::wstring{ path });
	}

	auto MinecraftBedrockGamePackagesViewModel::SupportsImporting() const noexcept -> bool {

		return packageSourceId == L"Imports";
	}

	auto MinecraftBedrockGamePackagesViewModel::CanStartImporting() const noexcept -> bool {

		return SupportsImporting();
	}

	auto MinecraftBedrockGamePackagesViewModel::InstallGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage, winrt::hstring const& installLocation) -> void {

		auto& settings = MinecraftBedrockGameManager::Settings();
		settings.GameInstallLocation(std::wstring{ installLocation });
		settings.Save();

		TryRegisterGamePackageStatusListener(gamePackage);
		MinecraftBedrockGameManager::InstallGamePackageAsync(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::InitiateGamePackageImport(winrt::hstring gamePackageLocation) -> winrt::IAsyncOperation<Citrine::MinecraftBedrockGamePackageImportContext> {

		auto cancellationToken = co_await winrt::get_cancellation_token();
		cancellationToken.enable_propagation();

		try {

			auto importContext = co_await MinecraftBedrockGameManager::InitiateGamePackageImportAsync(std::wstring{ gamePackageLocation });
			if (!importContext) {

				ToastNotificationService::SendNotification(NotificationSeverity::Error, Localizer::GetString(L"ErrorMessage_OpeningGamePackageFailed"), {});
				co_return nullptr;
			}
			co_return importContext;
		}
		catch (...) {
		
			co_return nullptr;
		}
	}

	auto MinecraftBedrockGamePackagesViewModel::ImportGamePackage(Citrine::MinecraftBedrockGamePackageImportContext const& context, winrt::hstring const& installLocation, winrt::hstring const& nameTag) -> void {

		auto& settings = MinecraftBedrockGameManager::Settings();
		settings.GameInstallLocation(std::wstring{ installLocation });
		settings.Save();

		MinecraftBedrockGameManager::ImportGamePackageAsync(context, nameTag);
	}

	auto MinecraftBedrockGamePackagesViewModel::RegisterGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> winrt::fire_and_forget {

		auto result = co_await MinecraftBedrockGameManager::RegisterGamePackageAsync(gamePackage);
		if (result == MinecraftBedrockGamePackageRegisterResult::Failed) {

			ToastNotificationService::SendNotification(NotificationSeverity::Error, Localizer::GetString(L"ErrorMessage_RegisteringGamePackageFailed"), {});
		}
	}

	auto MinecraftBedrockGamePackagesViewModel::LaunchGamePackage(Citrine::MinecraftBedrockGameLaunchArgs launchArgs) -> winrt::fire_and_forget {

		auto result = co_await MinecraftBedrockGameManager::LaunchGamePackageAsync(launchArgs);
		if (result == MinecraftBedrockGameLaunchResult::Failed) {

			ToastNotificationService::SendNotification(NotificationSeverity::Error, Localizer::GetString(L"ErrorMessage_LaunchingGameFailed"), {});
		}
	}

	auto MinecraftBedrockGamePackagesViewModel::RenameGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage, winrt::hstring const& nameTag) -> void {

		MinecraftBedrockGameManager::RenameGamePackage(gamePackage, nameTag);
		filteredGamePackages.Refresh(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::UnregisterGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		MinecraftBedrockGameManager::UnregisterGamePackageAsync(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::UninstallGamePackage(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		TryRegisterGamePackageStatusListener(gamePackage);
		MinecraftBedrockGameManager::UninstallGamePackageAsync(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::OpenGameDirectory(Citrine::MinecraftBedrockGamePackageItem gamePackage) -> winrt::fire_and_forget {

		using ::Citrine::Windows::Shell;
		co_await Shell::OpenFolderAsync(MinecraftBedrockGameManager::GetGameDirectory(gamePackage));
	}

	auto MinecraftBedrockGamePackagesViewModel::OpenGameDataDirectory(Citrine::MinecraftBedrockGamePackageItem gamePackage) -> winrt::fire_and_forget {

		using ::Citrine::Windows::Shell;
		co_await Shell::OpenFolderAsync(MinecraftBedrockGameManager::GetGameDataDirectory(gamePackage));
	}

	auto MinecraftBedrockGamePackagesViewModel::PauseGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		MinecraftBedrockGameManager::PauseGamePackageOperation(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::ResumeGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		MinecraftBedrockGameManager::ResumeGamePackageOperation(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::CancelGamePackageOperation(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		MinecraftBedrockGameManager::CancelGamePackageOperation(gamePackage);
	}

	auto MinecraftBedrockGamePackagesViewModel::PackageSourceId() const noexcept -> winrt::hstring {

		return packageSourceId;
	}

	auto MinecraftBedrockGamePackagesViewModel::RegisterGamePackageStatusListener(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> void {

		using enum GamePackageStatus;

		auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
		auto status = gamePackageImpl->Status();
		
		struct Listener {

			auto Invoke(winrt::IInspectable const& sender, winrt::PropertyChangedEventArgs const& args) -> void {

				if (args.PropertyName() != L"Status")
					return;

				auto gamePackageImpl = sender.as<GamePackageItemImpl>();
				auto status = gamePackageImpl->Status();
				auto prevStatus = std::exchange(PreviousStatus, status);

				if (status == NotInstalled || status == Installed) {

					gamePackageImpl->PropertyChanged(Token);
				}
				else if (prevStatus != NotInstalled && prevStatus != Installed) {

					return;
				}

				if (auto collection = FilteredCollection.get())
					collection.Refresh(*gamePackageImpl);
			}

			GamePackageStatus PreviousStatus{};
			winrt::weak_ref<Citrine::IFilterableCollectionView> FilteredCollection;
			winrt::event_token Token;
		};

		auto listener = std::make_shared<Listener>(status, filteredGamePackages);
		listener->Token = gamePackageImpl->PropertyChanged({ auto{ listener }, &Listener::Invoke });
	}

	auto MinecraftBedrockGamePackagesViewModel::TryRegisterGamePackageStatusListener(Citrine::MinecraftBedrockGamePackageItem const& gamePackage) -> bool {

		using enum GamePackageStatus;

		if (SupportsImporting())
			return false;

		auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
		auto status = gamePackageImpl->Status();

		if (status != NotInstalled && status != Installed)
			return false;

		RegisterGamePackageStatusListener(gamePackage);
		return true;
	}

	Citrine::IItemFilter MinecraftBedrockGamePackagesViewModel::installedGamePackagesFilter{ nullptr };
}
