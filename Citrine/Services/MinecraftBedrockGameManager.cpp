#include "pch.h"
#include "MinecraftBedrockGameManager.h"

#include "Minecraft/Bedrock/GamePackage.h"
#include "Minecraft/Bedrock/GamePackageMeta.h"
#include "Minecraft/Bedrock/ServerPackageMeta.h"
#include "Minecraft/Bedrock/GamePackageOperation.h"
#include "Models/MinecraftBedrockGamePackageItem.h"
#include "Models/MinecraftBedrockGamePackageImportContext.h"
#include "Models/MinecraftBedrockGameLaunchArgs.h"

#include "Core/Coroutine/FireAndForget.h"
#include "Core/Util/Scope.h"
#include "Core/Util/Ascii.h"
#include "Core/Unicode/Utf.h"
#include "Core/Logging/Logger.h"
#include "Core/IO/File.h"
#include "Collections/ObservableCollection.h"
#include "ApplicationData.h"
#include "Services/HttpService.h"
#include "Xbox/Xvc/StreamedXvcFile.h"
#include "Xbox/Keys/KeyRegistry.h"
#include "Windows/Fe3Handler.h"
#include "Windows/Msix/StreamedMsixFile.h"
#include "Windows/Msix/MsixManifest.h"
#include "Windows/Shell.h"
#include "Windows/AppLauncher.h"
#include "Windows/User.h"

#include <atomic>
#include <filesystem>
#include <variant>
#include <array>
#include <bit>
#include <semaphore>
#include <optional>

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Management.Core.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.h>

#include <tlhelp32.h>
#include <processthreadsapi.h>
#include <winuser.h>
#include <synchapi.h>

#include <wil/resource.h>

#include <glaze/json.hpp>

using namespace Citrine;
using namespace Minecraft::Bedrock;

using namespace std::string_view_literals;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Storage;
	using namespace Windows::Management::Core;
	using namespace Windows::Management::Deployment;
	using namespace Windows::ApplicationModel;
	using namespace Microsoft::UI::Xaml;
}

namespace {

	using GamePackageItem = winrt::Citrine::MinecraftBedrockGamePackageItem;
	using GamePackageItemImpl = winrt::Citrine::implementation::MinecraftBedrockGamePackageItem;
	using GamePackageStatus = winrt::Citrine::MinecraftBedrockGamePackageStatus;
	using GamePackageImportContext = winrt::Citrine::MinecraftBedrockGamePackageImportContext;
	using GamePackageImportContextImpl = winrt::Citrine::implementation::MinecraftBedrockGamePackageImportContext;
	using GameLaunchArgs = winrt::Citrine::MinecraftBedrockGameLaunchArgs;
	using GameLaunchArgsImpl = winrt::Citrine::implementation::MinecraftBedrockGameLaunchArgs;
	using GameLaunchResult = winrt::Citrine::MinecraftBedrockGameLaunchResult;
	using winrt::Citrine::InstallLocationValidationResult;

	struct GamePackageItemGreater {

		static auto operator()(GamePackageItem const& left, GamePackageItem const& right) noexcept -> bool {

			return std::is_gt(GetImpl(left).CompareTo(GetImpl(right)));
		}

		static auto GetImpl(GamePackageItem const& item) noexcept -> GamePackageItemImpl const& {

			return *winrt::get_self<GamePackageItemImpl>(item);
		}
	};

	struct alignas(16) Progress {

		std::uint64_t Status : 8 {};
		std::uint64_t BytesProcessed : 56 {};
		std::uint64_t TotalBytesToProcess : 56 {};
		std::uint64_t HasValue : 8 {};
	};

	class ProgressToken {
	public:

		ProgressToken(ProgressToken const&) noexcept = default;
		auto operator=(ProgressToken const&) noexcept -> ProgressToken& = default;

		auto operator()(GamePackageStatus status) noexcept -> void {

			auto progressValue = Progress{

				.Status = static_cast<std::uint64_t>(status),
				.HasValue = true
			};
			std::atomic_ref{ *ptr }.store(progressValue, std::memory_order::release);
		}

		auto operator()(GamePackageStatus status, std::uint64_t bytesProcessed, std::uint64_t totalBytesToProcess) noexcept -> void {

			auto progressValue = Progress{

				.Status = static_cast<std::uint64_t>(status),
				.BytesProcessed = bytesProcessed,
				.TotalBytesToProcess = totalBytesToProcess,
				.HasValue = true
			};
			std::atomic_ref{ *ptr }.store(progressValue, std::memory_order::release);
		}

	private:

		friend class ProgressDispatcher;

		ProgressToken(Progress* ptr) noexcept

			: ptr(ptr)
		{}

		Progress* ptr{ nullptr };
	};

	class ProgressDispatcher {
	public:

		ProgressDispatcher() = default;

		ProgressDispatcher(ProgressDispatcher const&) = delete;
		auto operator=(ProgressDispatcher const&) = delete;

		auto Initialize() -> void {

			timer = {};
			timer.Interval(50ms);
			timer.Tick([this](auto const&...) { OnTimerTick(); });
		}

		auto Register(GamePackageItem gamePackage) -> ProgressToken {

			auto token = ProgressToken{ entries.emplace_back(gamePackage, new Progress{}).Progress };
			if (!timer.IsEnabled())
				timer.Start();
			return token;
		}

		auto Sync(ProgressToken token) -> void {

			auto it = std::ranges::find(entries, token.ptr, &Entry::Progress);
			if (it != entries.end()) {

				auto const& [status, bytesProcessed, totalBytesToProcess, hasValue] = std::atomic_ref{ *it->Progress }.load(std::memory_order::acquire);
				if (!hasValue)
					return;

				auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(it->GamePackage);
				gamePackageImpl->Status(static_cast<GamePackageStatus>(status));
				gamePackageImpl->OperationProgress({ bytesProcessed, totalBytesToProcess });
			}
		}

		auto Unregister(ProgressToken token) -> void {

			auto it = std::ranges::find(entries, token.ptr, &Entry::Progress);
			if (it != entries.end()) {

				entries.erase(it);
				delete token.ptr;
			}

			if (entries.empty())
				timer.Stop();
		}

		auto Shutdown() -> void {

			timer.Stop();
		}

		~ProgressDispatcher() {

			Shutdown();
		}

	private:

		auto OnTimerTick() -> void {

			for (auto& entry : entries) {

				auto const& [status, bytesProcessed, totalBytesToProcess, hasValue] = std::atomic_ref{ *entry.Progress }.load(std::memory_order::acquire);
				if (!hasValue)
					continue;

				auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(entry.GamePackage);
				gamePackageImpl->Status(static_cast<GamePackageStatus>(status));
				gamePackageImpl->OperationProgress({ bytesProcessed, totalBytesToProcess });
			}
		}

		struct Entry {

			GamePackageItem GamePackage{ nullptr };
			Progress* Progress{ nullptr };
		};

		std::vector<Entry> entries;
		winrt::DispatcherTimer timer{ nullptr };
	};

	struct KnownPackageFamily {

		constexpr KnownPackageFamily(auto const& name) noexcept

			: Name(name)
		{}

		auto operator==(this KnownPackageFamily const& left, KnownPackageFamily const& right) -> bool {

			constexpr auto toLower = [](auto ch) static { return Ascii::ToLower(ch); };
			return std::ranges::equal(left.Name, right.Name, {}, toLower, toLower);
		}

		std::string_view Name;
	};

	struct KnownPackageFamilies {

		static constexpr auto MinecraftUWP = KnownPackageFamily{ "Microsoft.MinecraftUWP_8wekyb3d8bbwe" };
		static constexpr auto MinecraftWindowsBeta = KnownPackageFamily{ "Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe" };
	};

	class PackageObject : public std::variant<std::monostate, Xbox::StreamedXvcFile, Windows::StreamedMsixFile> {
	public:

		using PackageObject::variant::variant;

		auto IsXvc() const noexcept -> bool {

			return index() == 1;
		}

		auto IsMsix() const noexcept -> bool {

			return index() == 2;
		}

		auto GetPackageManifest() const noexcept -> Windows::MsixManifest const* {

			if (auto xvc = std::get_if<Xbox::StreamedXvcFile>(this))
				return &xvc->PackageManifest();
			else if (auto msix = std::get_if<Windows::StreamedMsixFile>(this))
				return &msix->PackageManifest();
			return nullptr;
		}
	};

	auto GetGameDirectoryName(GamePackageIdentity const& id) -> std::wstring {

		using enum GameBuildType;
		auto nameLength = 0uz;

		constexpr auto prefix = L"MC"sv;
		nameLength += prefix.size();

		auto versionBuffer = TrivialArray<wchar_t, VersionNumberFormatter::MaxFormattedSize(id.Version)>{};
		auto version = std::wstring_view{ versionBuffer.data(), VersionNumberFormatter::FormatTo(versionBuffer.data(), id.Version) };
		nameLength += 1;
		nameLength += version.size();

		auto suffix = std::wstring_view{};
		switch (id.BuildType) {
		case Preview:	suffix = L"Preview";	break;
		case Beta:		suffix = L"Beta";		break;
		}

		if (suffix.size() > 0) {

			nameLength += 1;
			nameLength += suffix.size();
		}

		auto installId = FormatInteger<wchar_t>(id.InstallId);
		if (id.InstallId > 0) {

			nameLength += 1;
			nameLength += installId.size();
		}

		auto name = std::wstring{};
		name.resize_and_overwrite(nameLength, [&](wchar_t* data, std::size_t size) {

			auto out = data;

			out = std::ranges::copy(prefix, out).out;
			*out++ = L'-';
			out = std::ranges::copy(version, out).out;

			if (suffix.size() > 0) {

				*out++ = L'-';
				out = std::ranges::copy(suffix, out).out;
			}

			if (id.InstallId > 0) {

				*out++ = L'_';
				out = std::ranges::copy(installId, out).out;
			}

			return size;
		});
		return name;
	}

	auto GetGameShortcutName(GamePackageIdentity const& id) -> std::wstring {

		using enum GameBuildType;
		auto nameLength = 0uz;

		constexpr auto prefix = L"Minecraft"sv;
		nameLength += prefix.size();

		auto versionBuffer = TrivialArray<wchar_t, VersionNumberFormatter::MaxFormattedSize(id.Version)>{};
		auto version = std::wstring_view{ versionBuffer.data(), VersionNumberFormatter::FormatTo(versionBuffer.data(), id.Version) };
		nameLength += 1;
		nameLength += version.size();

		auto suffix = std::wstring_view{};
		switch (id.BuildType) {
		case Preview:	suffix = L"Preview";	break;
		case Beta:		suffix = L"Beta";		break;
		}

		if (suffix.size() > 0) {

			nameLength += 1;
			nameLength += suffix.size();
		}

		constexpr auto extension = L".lnk"sv;
		nameLength += extension.size();

		auto name = std::wstring{};
		name.resize_and_overwrite(nameLength, [&](wchar_t* data, std::size_t size) {

			auto out = data;

			out = std::ranges::copy(prefix, out).out;
			*out++ = L' ';
			out = std::ranges::copy(version, out).out;

			if (suffix.size() > 0) {

				*out++ = L' ';
				out = std::ranges::copy(suffix, out).out;
			}

			out = std::ranges::copy(extension, out).out;

			return size;
		});
		return name;
	}

	constexpr auto& ExtractionContextFileName = L"Citrine.ExtractionContext.bin";

	class GameManagerInternal {
	public:

		auto InitializeAsync() -> FireAndForget {

			Logger::Info("Initializing MinecraftBedrockGameManager");

			rootDirectory = ApplicationData::LocalDirectory() / L"Bedrock";
			std::filesystem::create_directory(rootDirectory);

			backupsDirectory = rootDirectory / L"Backups";
			std::filesystem::create_directory(backupsDirectory);

			settings.Open(rootDirectory / L"Settings.json");
			{
				using enum InstallLocationValidationResult;

				auto write = !settings.Load();
				if (ValidateInstallLocation(settings.GameInstallLocation()) != Success) {

					settings.GameInstallLocation(rootDirectory);
					write = true;
				}

				if (write)
					settings.Save();
			}

			progressDispatcher.Initialize();

			auto context = winrt::apartment_context{};
			co_await winrt::resume_background();

			auto loadMetaAsync = [this](this auto self, auto& meta, std::filesystem::path fileName) -> Task<> {

				constexpr auto& baseUrl = "https://raw.githubusercontent.com/CitrineLauncher/VersionIndex/main/Bedrock/";
				auto result = co_await HttpService::SendRequestAsync(HttpMethod::Get, UrlCombine(baseUrl, fileName));

				auto file = File{ rootDirectory / fileName, FileMode::OpenAlways, FileAccess::ReadWrite };
				if (result) {

					auto& response = *result;

					constexpr auto opts = glz::opts{ .null_terminated = false, .error_on_unknown_keys = false };
					auto ec = glz::read<opts>(meta, response.Content);
					if (!ec) {

						file.Write(response.Content);
						file.Truncate();
						co_return;
					}
				}

				auto buffer = std::string{};
				file.ReadToEnd(buffer);

				constexpr auto opts = glz::opts{ .error_on_unknown_keys = false };
				auto ec = glz::read<opts>(meta, buffer);
				if (!ec) {

					co_return;
				}

				Logger::Error("Loading {} failed", fileName);
			};

			auto loadGameMetaTask = loadMetaAsync(gameMeta, L"GamePackages.json");
			auto loadServerMetaTask = loadMetaAsync(serverMeta, L"ServerPackages.json");

			auto releases = std::vector<GamePackageItem>{};
			auto previews = std::vector<GamePackageItem>{};
			auto imports = std::vector<GamePackageItem>{};

			gameInstallations.Open(rootDirectory / L"GameInstallations.json");
			gameInstallations.Load([this](StorageOperationResult result, std::string const& buffer) {

				if (result)
					return;

				if (result == StorageError::DeserializationFailed) {

					if (buffer.empty())
						return;

					Logger::Error("Parsing game installations failed, creating backup");

					auto backupFilePath = backupsDirectory / L"GameInstallations.bak";
					if (RotateFile(backupFilePath, 32) && WriteFile(backupFilePath, buffer)) {

						gameInstallations.Clear();
						Logger::Info("Creating backup of game installations completed");
						return;
					}

					Logger::Error("Creating backup of game installations failed");
				}
				else {

					Logger::Error("Loading game installations failed");
				}

				gameInstallations.Close(); // Close file to avoid data loss
			});

			{
				auto write = false;
				auto it = gameInstallations->begin();

				while (it != gameInstallations->end()) {

					auto& package = *it;

					auto gameDirectory = package.InstallLocation / GetGameDirectoryName(package);
					if (auto ec = std::error_code{}; !std::filesystem::is_directory(gameDirectory, ec)) {

						Logger::Error("Root directory ({}) of game package {} not found: {}", gameDirectory, package, ec.value());
						if (ec.value() == ERROR_FILE_NOT_FOUND || ec.value() == ERROR_PATH_NOT_FOUND) {

							it = gameInstallations->Remove(it);
							write = true;
							continue;
						}
					}

					auto& destination = package.Origin == GamePackageOrigin::Meta
						? (package.BuildType == GameBuildType::Preview ? previews : releases)
						: (imports);

					destination.emplace_back(winrt::make<GamePackageItemImpl>(
						package,
						winrt::to_hstring(package.NameTag),
						CheckGamePackageCompatibility(package),
						true,
						CheckGameCapabilities(package),
						GamePackageStatus::Installed
					));
					++it;
				}

				if (write)
					gameInstallations.Save();
			}

			auto packagesInUse = std::vector<GamePackageItem>{};

			gamePackageOperations.Open(rootDirectory / "GamePackageOperations.json");
			gamePackageOperations.Load();

			{
				constexpr auto packageEqual = GamePackageEqualityComparer{};

				auto write = false;
				auto it = gamePackageOperations->begin();

				while (it != gamePackageOperations->end()) {

					auto& op = *it;
					auto package = std::get_if<GamePackage>(&op.Package);

					if (!package) {

						it = gamePackageOperations->Remove(it);
						write = true;
						continue;
					}

					auto& installLocation = package->InstallLocation;
					if (auto ec = std::error_code{}; !std::filesystem::is_directory(installLocation, ec)) {

						Logger::Error("Install location ({}) of game package {} not found: {}", installLocation, *package, ec.value());
						if (ec.value() == ERROR_FILE_NOT_FOUND || ec.value() == ERROR_PATH_NOT_FOUND) {

							it = gamePackageOperations->Remove(it);
							write = true;
							continue;
						}
					}

					auto& destination = package->Origin == GamePackageOrigin::Meta
						? (package->BuildType == GameBuildType::Preview ? previews : releases)
						: (imports);

					auto destPos = destination.begin();
					while (destPos != destination.end()) {

						auto& itemImpl = *winrt::get_self<GamePackageItemImpl>(*destPos);
						if (itemImpl.Id().Version <= package->Version)
							break;

						++destPos;
					}

					auto packageFound = false;
					while (destPos != destination.end()) {

						auto& itemImpl = *winrt::get_self<GamePackageItemImpl>(*destPos);
						if (itemImpl.Id().Version != package->Version)
							break;

						if (packageEqual(itemImpl.Id(), *package)) {

							packageFound = true;
							break;
						}
						++destPos;
					}

					auto packageStatus = GamePackageStatus{};
					auto cancellationRequested = op.Status == PackageOperationStatus::CancellationRequested;
					auto valid = false;

					switch (op.Action) {
					case PackageAction::Install: [[fallthrough]];
					case PackageAction::Import: {

						packageStatus = cancellationRequested
							? GamePackageStatus::CancellingInstallation
							: GamePackageStatus::InstallationPaused;
						valid = true;
					} break;
					case PackageAction::Uninstall: {

						packageStatus = GamePackageStatus::UninstallationPending;
						valid = true;
					} break;
					}

					if (packageFound || !valid) {

						Logger::Error("Encountered invalid operation ({}) for game package {}", op.Action, *package);
						it = gamePackageOperations->Remove(it);
						write = true;
						continue;
					}

					auto& item = *destination.emplace(destPos, winrt::make<GamePackageItemImpl>(
						*package,
						winrt::to_hstring(package->NameTag),
						CheckGamePackageCompatibility(*package),
						false,
						CheckGameCapabilities(*package),
						packageStatus
					));

					if (op.Action == PackageAction::Import || op.Action == PackageAction::Uninstall || cancellationRequested)
						packagesInUse.emplace_back(item);
					else
						op.Status = PackageOperationStatus::Paused;

					++it;
				}

				if (write)
					gamePackageOperations.Save();
			}
			std::ranges::sort(imports, GamePackageItemGreater{});

			// Current workaround for disabling texture streaming
			auto deleteProgressionsFile = [&currentUser = Windows::GetCurrentUser()](std::wstring_view rootDirName, File& progressionsFile) {

				auto ec = std::error_code{};
				auto flightingDirectory = currentUser.RoamingAppDataDirectory / rootDirName / L"Flighting";
				std::filesystem::create_directories(flightingDirectory, ec);

				progressionsFile.Open(
					flightingDirectory / L"currentProgressions",
					FileMode::OpenAlways,
					FileAccess::Delete,
					FileShare::ReadWrite | FileShare::Delete
				);
				progressionsFile.Delete();
			};
			deleteProgressionsFile(L"Minecraft Bedrock", releaseBuildProgressionsFile);
			deleteProgressionsFile(L"Minecraft Bedrock Preview", previewBuildProgressionsFile);

			co_await std::move(loadGameMetaTask);

			auto populateGamePackages = [&source = gameMeta.Packages](std::vector<GamePackageItem>& destination, GameBuildType buildType) {

				struct SelectedPackageInfo {

					operator bool() const noexcept {

						return static_cast<bool>(PackageInfo);
					}

					GamePackageInfo const* PackageInfo{ nullptr };
					GamePackageCompatibility Compatibility;
				};
				constexpr auto packageEqual = GamePackageEqualityComparer{};

				auto it = source.begin();
				auto destPos = 0uz;

				while (it != source.end()) {

					auto version = it->Version;
					auto selectedPackage = SelectedPackageInfo{};

					do {

						if (it->BuildType != buildType)
							continue;

						auto compatibility = CheckGamePackageCompatibility(it->Version, it->Platform, it->Architecture);
						if (compatibility > selectedPackage.Compatibility) {

							selectedPackage.PackageInfo = &*it;
							selectedPackage.Compatibility = compatibility;
						}

					} while (++it != source.end() && it->Version == version);

					if (!selectedPackage)
						continue;

					auto& packageInfo = *selectedPackage.PackageInfo;
					auto packageId = GamePackageIdentity{

						.Version = packageInfo.Version,
						.BuildType = packageInfo.BuildType,
						.Platform = packageInfo.Platform,
						.Architecture = packageInfo.Architecture,
						.Origin = GamePackageOrigin::Meta,
						.InstallId = 0
					};
					auto packageCompatibility = selectedPackage.Compatibility;

					while (destPos < destination.size()) {

						auto& itemImpl = *winrt::get_self<GamePackageItemImpl>(destination[destPos]);
						if (itemImpl.Id().Version <= version)
							break;

						++destPos;
					}

					auto packageFound = false;
					while (destPos < destination.size()) {

						auto& itemImpl = *winrt::get_self<GamePackageItemImpl>(destination[destPos]);
						if (itemImpl.Id().Version != version)
							break;

						if (packageEqual(itemImpl.Id(), packageId)) {

							packageFound = true;
							break;
						}
						++destPos;
					}

					if (!packageFound) {

						destination.emplace(destination.begin() + destPos, winrt::make<GamePackageItemImpl>(
							packageId,
							L"",
							packageCompatibility,
							false,
							CheckGameCapabilities(packageId),
							GamePackageStatus::NotInstalled
						));
						++destPos;
					}
				}
			};
			populateGamePackages(releases, GameBuildType::Release);
			populateGamePackages(previews, GameBuildType::Preview);
			std::ranges::sort(releases, GamePackageItemGreater{});
			std::ranges::sort(previews, GamePackageItemGreater{});

			co_await std::move(loadServerMetaTask);
			co_await context;

			releaseGamePackages->Underlying(std::move(releases));
			previewGamePackages->Underlying(std::move(previews));
			importedGamePackages->Underlying(std::move(imports));

			for (auto& item : packagesInUse) {

				auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
				auto packageId = itemImpl->Id();

				auto op = gamePackageOperations->Find(packageId);
				if (op->Action == PackageAction::Install || op->Action == PackageAction::Import) {

					ExecuteInstallGamePackageOperation(std::move(item), std::to_address(op));
				}
				else if (op->Action == PackageAction::Uninstall) {

					ExecuteUninstallGamePackageOperation(std::move(item), std::to_address(op));
				}
			}

			initialized = true;
			initializationCompletedEvent();
		}

		auto InitializationCompleted(EventHandler<>&& handler) -> EventToken {

			if (initialized) {

				handler();
				return {};
			}
			return initializationCompletedEvent.Add(std::move(handler));
		}

		auto InitializationCompleted(EventToken&& token) -> void {

			initializationCompletedEvent.Remove(std::move(token));
		}

		auto Settings() -> MinecraftBedrockGameManagerSettings& {

			return settings;
		}

		auto ReleaseGamePackages() -> winrt::Citrine::IObservableCollectionView {

			return *releaseGamePackages;
		}

		auto PreviewGamePackages() -> winrt::Citrine::IObservableCollectionView {

			return *previewGamePackages;
		}

		auto ImportedGamePackages() -> winrt::Citrine::IObservableCollectionView {

			return *importedGamePackages;
		}

		auto ValidateInstallLocation(std::filesystem::path const& path) -> InstallLocationValidationResult {

			using enum InstallLocationValidationResult;

			if (path.is_relative())
				return InvalidPath;

			auto maxPathLength = MAX_PATH - (1 + 24 + 1 + 144);
			auto& nativePath = path.native();

			if (nativePath.back() == '\\' || nativePath.back() == '/')
				++maxPathLength;

			if (nativePath.size() > maxPathLength)
				return MaxPathLengthExceeded;

			auto ec = std::error_code{};
			if (!std::filesystem::is_directory(path, ec))
				return DirectoryNotFound;

			return Success;
		}

		auto InstallGamePackageAsync(GamePackageItem&& item) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			if (gameInstallations->Contains(packageId)) {

				Logger::Warn("Game package {} is already installed", packageId);
				return;
			}

			auto [op, inserted] = gamePackageOperations->Emplace(
				GamePackage{ packageId, settings.GameInstallLocation() },
				L"",
				PackageAction::Install
			);
			if (!inserted) {

				Logger::Warn("Game package {} is already in use", packageId);
				return;
			}
			gamePackageOperations.Save();

			ExecuteInstallGamePackageOperation(std::move(item), std::to_address(op));
		}

		auto InitiateGamePackageImportAsync(std::filesystem::path gamePackageLocation) -> Task<GamePackageImportContext> {

			if (!initialized)
				co_return nullptr;

			Logger::Info("Initiating import of game package ({})", gamePackageLocation);

			auto extension = gamePackageLocation.extension().string();
			std::ranges::transform(extension, extension.data(), [](char ch) static { return Ascii::ToLower(ch); });

			if (extension != ".msixvc" && extension != ".appx" && extension != ".msix") {

				Logger::Error("Initiating import of game package ({}) failed: unsupported file extension", gamePackageLocation);
				co_return nullptr;
			}

			auto packageFile = File{ gamePackageLocation, FileMode::OpenExisting, FileAccess::Read };
			if (!packageFile) {

				Logger::Error("Initiating import of game package ({}) failed: package file opening failed ({})", gamePackageLocation, packageFile.LastError());
				co_return nullptr;
			}

			auto packageObject = PackageObject{};
			if (extension == ".msixvc") {

				auto xvcFile = co_await Xbox::StreamedXvcFile::OpenFromFileAsync(std::move(packageFile));
				if (!xvcFile) {

					Logger::Error("Initiating import of game package ({}) failed: package opening failed ({})", gamePackageLocation, xvcFile.error());
					co_return nullptr;
				}
				packageObject = std::move(*xvcFile);
			}
			else {
				
				auto msixFile = co_await Windows::StreamedMsixFile::OpenFromFileAsync(std::move(packageFile));
				if (!msixFile) {

					Logger::Error("Initiating import of game package ({}) failed: package opening failed ({})", gamePackageLocation, msixFile.error());
					co_return nullptr;
				}
				packageObject = std::move(*msixFile);
			}

			auto packageId = GamePackageIdentity{};

			auto& packageManifest = *packageObject.GetPackageManifest();
			auto packageManifestId = packageManifest.Identity();

			if (!packageManifestId.IsValid()) {

				Logger::Error("Initiating import of game package ({}) failed: invalid identity ({})", gamePackageLocation, packageManifestId);
				co_return nullptr;
			}

			packageId.Version = GameVersion::FromWindowsAppPackageVersion(packageManifestId.Version());

			auto packageFamily = Windows::GetPackageFamilyNameFromId(packageManifestId);
			if (packageFamily == KnownPackageFamilies::MinecraftUWP) {

				packageId.BuildType = GameBuildType::Release;
				packageId.Platform = packageObject.IsXvc()
					? GamePlatform::WindowsGDK
					: GamePlatform::WindowsUWP;
			}
			else if (packageFamily == KnownPackageFamilies::MinecraftWindowsBeta) {

				packageId.BuildType = GameBuildType::Preview;
				packageId.Platform = packageObject.IsXvc()
					? GamePlatform::WindowsGDK
					: GamePlatform::WindowsUWP;
			}
			else {

				Logger::Error("Initiating import of game package ({}) failed: unknown package family ({})", gamePackageLocation, packageFamily);
				co_return nullptr;
			}

			packageId.Architecture = packageManifestId.Architecture();
			packageId.Origin = GamePackageOrigin::Import;

			auto compatibility = CheckGamePackageCompatibility(packageId);
			if (!compatibility) {

				Logger::Error("Initiating import of game package ({}) failed: package is incompatible (Platform: {}, Arch: {})", gamePackageLocation, packageId.Platform, packageId.Architecture);
				co_return nullptr;
			}

			auto bitSet = std::array<std::uint64_t, 2>{};
			bitSet[0] |= 1;

			for (auto const& otherPackage : gameInstallations->FindByVersion(packageId.Version)) {

				auto buildType = otherPackage.BuildType;
				if (buildType != packageId.BuildType)
					continue;

				auto installId = otherPackage.InstallId;
				if (installId < 100) {

					auto wordIndex = installId / 64;
					auto bitIndex = installId - (wordIndex * 64);
					bitSet[wordIndex] |= std::uint64_t{ 1 } << bitIndex;
				}
			}

			for (auto const& op : *gamePackageOperations) {

				auto otherPackage = std::get_if<GamePackage>(&op.Package);
				if (!otherPackage)
					continue;

				auto version = otherPackage->Version;
				if (version != packageId.Version)
					continue;

				auto buildType = otherPackage->BuildType;
				if (buildType != packageId.BuildType)
					continue;

				auto installId = otherPackage->InstallId;
				if (installId < 100) {

					auto wordIndex = installId / 64;
					auto bitIndex = installId - (wordIndex * 64);
					bitSet[wordIndex] |= std::uint64_t{ 1 } << bitIndex;
				}
			}

			packageId.InstallId = std::countr_one(bitSet[0]);
			if (packageId.InstallId == 64)
				packageId.InstallId += std::countr_one(bitSet[1]);

			if (packageId.InstallId >= 100) {

				Logger::Error("Initiating import of game package ({}) failed: package duplicate limit exceeded", gamePackageLocation);
				co_return nullptr;
			}

			auto nameTag = winrt::hstring{};
			if (packageId.InstallId >= 2)
				nameTag = winrt::format(L"({})", packageId.InstallId);

			auto item = winrt::make<GamePackageItemImpl>(
				packageId,
				std::move(nameTag),
				compatibility,
				false,
				CheckGameCapabilities(packageId),
				GamePackageStatus::NotInstalled
			);

			Logger::Info("Initiating import of game package ({}) completed, package id: {}", gamePackageLocation, packageId);
			co_return winrt::make<GamePackageImportContextImpl>(std::move(item), std::move(gamePackageLocation));
		}

		auto ImportGamePackageAsync(GamePackageImportContext&& importContext, winrt::hstring&& nameTag) -> void {

			if (!initialized)
				return;

			auto importContextImpl = winrt::get_self<GamePackageImportContextImpl>(importContext);
			auto item = importContextImpl->GamePackage();

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			if (gameInstallations->Contains(packageId)) {

				Logger::Warn("Game package {} is already installed", packageId);
				return;
			}

			auto [op, inserted] = gamePackageOperations->Emplace(
				GamePackage{ packageId, settings.GameInstallLocation(), ToUtf8(nameTag) },
				importContextImpl->GamePackageLocation(),
				PackageAction::Import
			);
			if (!inserted) {

				Logger::Warn("Game package {} is already in use", packageId);
				return;
			}
			gamePackageOperations.Save();

			{
				itemImpl->NameTag(nameTag);

				auto& underlying = importedGamePackages->Underlying();
				auto index = std::ranges::upper_bound(underlying, item, GamePackageItemGreater{}) - underlying.begin();

				importedGamePackages->InsertAt(static_cast<std::uint32_t>(index), item);
			}
			ExecuteInstallGamePackageOperation(std::move(item), std::to_address(op));
		}

		auto LaunchGamePackageAsync(GameLaunchArgs&& launchArgs) -> Task<GameLaunchResult> {

			if (!initialized)
				co_return GameLaunchResult::Blocked;

			auto launchArgsImpl = winrt::get_self<GameLaunchArgsImpl>(launchArgs);
			auto item = launchArgsImpl->SelectedGamePackage();

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			if (!gameInstallations->Contains(packageId)) {

				Logger::Error("Launching game package {} failed: not found", packageId);
				co_return GameLaunchResult::Failed;
			}

			auto [op, inserted] = gamePackageOperations->Emplace(
				packageId,
				L"",
				PackageAction::Launch
			);
			if (!inserted) {

				Logger::Warn("Game package {} is already in use", packageId);
				co_return GameLaunchResult::Failed;
			}

			co_return co_await ExecuteLaunchGamePackageOperation(std::move(launchArgs), std::to_address(op));
		}

		auto RenameGamePackage(GamePackageItem const& item, winrt::hstring const& nameTag) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto gamePackage = gameInstallations->Find(packageId);
			if (gamePackage == gameInstallations->end()) {

				Logger::Error("Renaming game package {} failed: not found", packageId);
				return;
			}

			gamePackage->NameTag = ToUtf8(nameTag);
			gameInstallations.Save();

			itemImpl->NameTag(nameTag);
			Logger::Info("Renaming game package {} completed", packageId);
		}

		auto GetGameDirectory(GamePackageItem const& item) -> std::filesystem::path {

			if (!initialized)
				return {};

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto gamePackage = gameInstallations->Find(packageId);
			if (gamePackage == gameInstallations->end())
				return {};

			return gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);
		}

		auto UninstallGamePackageAsync(GamePackageItem&& item) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			if (gamePackageOperations->Contains(packageId)) {

				Logger::Warn("Game package {} is already in use", packageId);
				return;
			}

			auto gamePackageNode = GamePackageCollection::NodeHandle{};
			{
				auto gamePackage = gameInstallations->Find(packageId);
				if (gamePackage == gameInstallations->end()) {

					Logger::Error("Uninstalling game package {} failed: not found", packageId);
					return;
				}

				gamePackageNode = gameInstallations->Extract(gamePackage);
			}

			auto [op, _] = gamePackageOperations->Emplace(
				std::move(gamePackageNode.value()),
				L"",
				PackageAction::Uninstall
			);
			gamePackageOperations.Save();
			gameInstallations.Save();

			ExecuteUninstallGamePackageOperation(std::move(item), std::to_address(op));
		}

		auto GetGameDataDirectory(GamePackageItem const& item) -> std::filesystem::path {

			if (!initialized)
				return {};

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto& currentUser = Windows::GetCurrentUser();

			if (packageId.Platform == GamePlatform::WindowsGDK) {

				auto directoryName = packageId.BuildType == GameBuildType::Preview
					? L"Minecraft Bedrock Preview"sv
					: L"Minecraft Bedrock"sv;

				return currentUser.RoamingAppDataDirectory / directoryName;
			}
			else if (packageId.Platform == GamePlatform::WindowsUWP) {

				auto directoryName = packageId.BuildType == GameBuildType::Preview
					? KnownPackageFamilies::MinecraftWindowsBeta.Name
					: KnownPackageFamilies::MinecraftUWP.Name;

				return currentUser.LocalAppDataDirectory / L"Packages" / directoryName / L"LocalState";
			}

			return {};
		}

		auto PauseGamePackageOperation(GamePackageItem&& item) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto op = gamePackageOperations->Find(packageId);
			if (op != gamePackageOperations->end()) {

				using enum PackageAction;
				using enum PackageOperationStatus;

				auto& action = op->Action;
				auto& status = op->Status;
				auto& task = op->Task;

				if (task && action == Install && status == Running) {

					status = PauseRequested;
					task.Cancel();
				}
			}
		}

		auto ResumeGamePackageOperation(GamePackageItem&& item) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			using enum PackageOperationStatus;

			auto op = gamePackageOperations->Find(packageId);
			if (op != gamePackageOperations->end()) {

				using enum PackageAction;
				using enum PackageOperationStatus;

				auto& action = op->Action;
				auto& status = op->Status;
				auto& task = op->Task;

				if (task)
					return;

				if ((action == Install || action == Import) && (status == Paused || status == Failed)) {

					ExecuteInstallGamePackageOperation(std::move(item), std::to_address(op));
				}
				else if (action == Uninstall && status == Failed) {

					ExecuteUninstallGamePackageOperation(std::move(item), std::to_address(op));
				}
			}
		}

		auto CancelGamePackageOperation(GamePackageItem&& item) -> void {

			if (!initialized)
				return;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto op = gamePackageOperations->Find(packageId);
			if (op != gamePackageOperations->end()) {

				using enum PackageAction;
				using enum PackageOperationStatus;

				auto& action = op->Action;
				auto& status = op->Status;
				auto& task = op->Task;

				if (action != Install && action != Import)
					return;

				if (task && (status == Running || status == PauseRequested)) {

					status = CancellationRequested;
					gamePackageOperations.Save();

					task.Cancel();
				}
				else if (!task && (status == Paused || status == Failed)) {

					status = CancellationRequested;
					gamePackageOperations.Save();

					ExecuteInstallGamePackageOperation(std::move(item), std::to_address(op));
				}
			}
		}

		auto Shutdown() -> void {

			if (!initialized)
				return;

			gameInstallations.Close();

			for (auto& operation : *gamePackageOperations) {

				using enum PackageAction;
				using enum PackageOperationStatus;

				auto& action = operation.Action;
				auto& status = operation.Status;
				auto& task = operation.Task;

				if (task && (action == Install || action == Import) && status == Running) {

					status = PauseRequested;
					task.Cancel(); // Invoke synchronous callback
				}
			}
			gamePackageOperations.Close();

			releaseBuildProgressionsFile.Close();
			previewBuildProgressionsFile.Close();

			progressDispatcher.Shutdown();
		}

	private:

		auto ExecuteInstallGamePackageOperation(GamePackageItem item, GamePackageOperation const* op) -> FireAndForget {

			using enum PackageAction;
			using enum GamePackageStatus;
			using enum PackageOperationStatus;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto clearTask = ScopeExit{ [&] { if (op) op->Task = nullptr; } };

			auto progressToken = progressDispatcher.Register(item);
			auto revokeProgressToken = ScopeExit{ [&] { progressDispatcher.Unregister(progressToken); } };

			auto gamePackage = std::get_if<GamePackage>(&op->Package);
			if (!gamePackage) {

				itemImpl->Status(InstallationFailed);
				Logger::Error("Installing game package {} failed: not found", packageId);
				co_return;
			}

			if (op->Status != CancellationRequested) try {

				op->Task = InstallGamePackageAsync(gamePackage, &op->PackageLocation, progressToken);
				op->Status = Running;
				progressDispatcher.Sync(progressToken);

				auto result = co_await std::move(op->Task);
				if (!result) {

					op->Status = Failed;
					itemImpl->Status(InstallationFailed);
					co_return;
				}

				auto opNode = gamePackageOperations->Extract(*std::exchange(op, nullptr));
				auto [packageIt, _] = gameInstallations->Insert(std::move(std::get<GamePackage>(opNode.value().Package)));
				gameInstallations.Save();
				gamePackageOperations.Save();

				gamePackage = std::to_address(packageIt);
				CleanupGamePackageInstallation(gamePackage);

				itemImpl->IsInstalled(true);
				itemImpl->Status(Installed);
				Logger::Info("Installation of game package {} completed", packageId);
				co_return;
			}
			catch (TaskCancelledException const&) {}

			if (op->Status == PauseRequested) {

				op->Status = Paused;
				itemImpl->Status(InstallationPaused);
				Logger::Info("Installation of game package {} paused", packageId);
			}
			else if (op->Status == CancellationRequested) {

				auto revertTask = RevertGamePackageInstallationAsync(gamePackage, progressToken);
				progressDispatcher.Sync(progressToken);

				co_await std::move(revertTask);
				gamePackageOperations->Remove(*std::exchange(op, nullptr));
				gamePackageOperations.Save();

				if (packageId.Origin == GamePackageOrigin::Import) {

					if (auto index = std::uint32_t{}; importedGamePackages->IndexOf(item, index))
						importedGamePackages->RemoveAt(index);
				}
				else {

					itemImpl->Status(NotInstalled);
				}
				Logger::Info("Installation of game package {} cancelled", packageId);
			}
		}

		auto ExecuteLaunchGamePackageOperation(GameLaunchArgs launchArgs, GamePackageOperation const* op) -> Task<GameLaunchResult> {

			using enum PackageAction;
			using enum GamePackageStatus;
			using enum PackageOperationStatus;

			auto launchArgsImpl = winrt::get_self<GameLaunchArgsImpl>(launchArgs);
			auto item = launchArgsImpl->SelectedGamePackage();

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto clearStatus = ScopeExit{ [&] { itemImpl->Status(Installed); } };
			auto removeOp = ScopeExit{ [&] { gamePackageOperations->Remove(*op); } };

			auto progressToken = progressDispatcher.Register(item);
			auto revokeProgressToken = ScopeExit{ [&] { progressDispatcher.Unregister(progressToken); } };

			auto gamePackage = gameInstallations->Find(packageId);
			if (gamePackage == gameInstallations->end()) {

				Logger::Error("Launching game package {} failed: not found", packageId);
				co_return GameLaunchResult::Failed;
			}

			auto& deploymentMutex = GetDeploymentMutex(*gamePackage);
			auto useDeploymentMutex = gamePackage->Platform != GamePlatform::WindowsGDK;

			if (useDeploymentMutex && !deploymentMutex.try_acquire())
				co_return GameLaunchResult::Blocked;

			auto releaseDeploymentMutex = ScopeExit{ [&] { if (useDeploymentMutex) deploymentMutex.release(); } };

			op->Task = LaunchGamePackageAsync(std::to_address(gamePackage), std::move(launchArgs), progressToken);
			op->Status = Running;
			progressDispatcher.Sync(progressToken);

			try {

				auto result = co_await std::move(op->Task);
				if (!result)
					co_return GameLaunchResult::Failed;

				Logger::Info("Launch of game package {} completed", packageId);
				co_return GameLaunchResult::Success;
			}
			catch (TaskCancelledException) {

				Logger::Error("Launch of game package {} cancelled", packageId);
				throw;
			}
		}

		auto ExecuteUninstallGamePackageOperation(GamePackageItem item, GamePackageOperation const* op) -> FireAndForget {

			using enum PackageAction;
			using enum GamePackageStatus;
			using enum PackageOperationStatus;

			auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
			auto packageId = itemImpl->Id();

			auto clearTask = ScopeExit{ [&] { if (op) op->Task = nullptr; } };

			auto progressToken = progressDispatcher.Register(item);
			auto revokeProgressToken = ScopeExit{ [&] { progressDispatcher.Unregister(progressToken); } };

			itemImpl->IsInstalled(false);

			auto gamePackage = std::get_if<GamePackage>(&op->Package);
			if (gamePackage) {

				op->Task = UninstallGamePackageAsync(std::to_address(gamePackage), progressToken);
				op->Status = Running;
				progressDispatcher.Sync(progressToken);

				auto result = co_await std::move(op->Task);
				if (!result) {

					op->Status = Failed;
					itemImpl->Status(UninstallationFailed);
					co_return;
				}
			}
			else {

				Logger::Warn("Removal of game package {} from system not handled", packageId);
			}

			gamePackageOperations->Remove(*std::exchange(op, nullptr));
			gamePackageOperations.Save();

			if (packageId.Origin == GamePackageOrigin::Import) {

				if (auto index = std::uint32_t{}; importedGamePackages->IndexOf(item, index))
					importedGamePackages->RemoveAt(index);
			}
			else {

				itemImpl->Status(NotInstalled);
			}

			Logger::Info("Uninstallation of game package {} completed", packageId);
			co_return;
		}

		auto InstallGamePackageAsync(GamePackage const* gamePackage, std::filesystem::path const* packageLocation, ProgressToken progressToken) -> Task<bool> {

			using enum GamePackageStatus;

			Logger::Info("Installing game package {}", *gamePackage);
			auto isImport = gamePackage->Origin == GamePackageOrigin::Import;

			if (gamePackage->Platform != GamePlatform::WindowsGDK && gamePackage->Platform != GamePlatform::WindowsUWP) {

				Logger::Error("Installing game package {} failed: unsupported platform", *gamePackage);
				co_return false;
			}

			progressToken(isImport ? Extracting : PreparingDownload);
			co_await winrt::resume_background();

			if (ValidateInstallLocation(gamePackage->InstallLocation) != InstallLocationValidationResult::Success) {

				Logger::Error("Installing game package {} failed: invalid install location ({})", *gamePackage, gamePackage->InstallLocation);
				co_return false;
			}

			auto gameDirectory = gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);
			if (auto ec = std::error_code{}; !std::filesystem::create_directory(gameDirectory) && ec) {

				Logger::Error("Installing game package {} failed: game directory creation failed ({})", *gamePackage, ec.value());
				co_return false;
			}

			auto contextFilePath = gameDirectory / ExtractionContextFileName;
			auto contextFile = File{ contextFilePath, FileMode::OpenAlways, FileAccess::ReadWrite };

			if (!contextFile) {

				Logger::Warn("Opening extraction context file ({}) for game package {} failed: {}", contextFilePath, *gamePackage, contextFile.LastError());
			}

			auto progressCallback = [progressToken, status = isImport ? Extracting : Downloading](auto const& progress) mutable {

				progressToken(status, progress.BytesProcessed, progress.TotalBytesToProcess);
			};

			auto packageInfo = static_cast<GamePackageInfo const*>(nullptr);
			auto packageFile = File{};

			if (isImport) {

				if (!packageFile.Open(*packageLocation, FileMode::OpenExisting, FileAccess::Read)) {

					Logger::Error("Installing game package {} failed: package file opening failed ({})", *gamePackage, packageFile.LastError());
					co_return false;
				}
			}
			else {

				auto it = gameMeta.Packages.Find(*gamePackage);
				if (it == gameMeta.Packages.end()) {

					Logger::Error("Installing game package {} failed: metadata not available", *gamePackage);
					co_return false;
				}
				packageInfo = std::to_address(it);
			}

			if (gamePackage->Platform == GamePlatform::WindowsGDK) {

				auto xvcFileResult = Xbox::XvcOperationResult<Xbox::StreamedXvcFile>{ std::unexpect };
				if (isImport) {

					xvcFileResult = co_await Xbox::StreamedXvcFile::OpenFromFileAsync(std::move(packageFile));
				}
				else {

					auto baseUrl = Url{};
					for (auto const& url : gameMeta.BaseUrls.WindowsGDK) {

						if (!url.IsWellFormed())
							continue;

						baseUrl = url;
						break;
					}

					if (!baseUrl.IsWellFormed()) {

						Logger::Error("Installing game package {} failed: no valid base url found", *gamePackage);
						co_return false;
					}

					auto packageStreamResponse = co_await HttpService::GetRandomAccessStreamAsync(UrlCombine(baseUrl, packageInfo->Path));
					if (!packageStreamResponse) {

						Logger::Error("Installing game package {} failed: package stream opening failed ({})", *gamePackage, packageStreamResponse.error());
						co_return false;
					}
					xvcFileResult = co_await Xbox::StreamedXvcFile::OpenFromStreamAsync(packageStreamResponse->Content.Stream());
				}

				if (!xvcFileResult) {

					Logger::Error("Installing game package {} failed: package opening failed ({})", *gamePackage, xvcFileResult.error());
					co_return false;
				}
				auto xvcFile = *std::move(xvcFileResult);

				auto cik = std::optional<Xbox::CikEntry>{};
				if (xvcFile.IsEncrypted()) {

					auto keyId = xvcFile.GetKeyId();
					auto cikEntry = Xbox::KeyRegistry::GetCik(keyId);

					if (!cikEntry) {

						Logger::Error("Installing game package {} failed: package cik ({}) not found", *gamePackage, keyId);
						co_return false;
					}
					cik.emplace(*cikEntry);
				}

				auto result = co_await xvcFile.ExtractAllFilesAsync(std::move(gameDirectory), cik, progressCallback, std::move(contextFile));
				if (!result) {

					Logger::Error("Installing game package {} failed: extraction failed ({})", *gamePackage, result.error());
					co_return false;
				}
			}
			else {

				auto msixFileResult = Windows::MsixOperationResult<Windows::StreamedMsixFile>{ std::unexpect };
				if (isImport) {

					msixFileResult = co_await Windows::StreamedMsixFile::OpenFromFileAsync(std::move(packageFile));
				}
				else {

					auto url = co_await Windows::FE3Handler::GetFileUrlAsync(packageInfo->UpdateId, 1);
					if (!url) {

						Logger::Error("Installing game package {} failed: fetching url failed", *gamePackage);
					}

					auto packageStreamResponse = co_await HttpService::GetRandomAccessStreamAsync(std::move(*url));
					if (!packageStreamResponse) {

						Logger::Error("Installing game package {} failed: package stream opening failed ({})", *gamePackage, packageStreamResponse.error());
						co_return false;
					}
					msixFileResult = co_await Windows::StreamedMsixFile::OpenFromStreamAsync(packageStreamResponse->Content.Stream());
				}

				if (!msixFileResult) {

					Logger::Error("Installing game package {} failed: package opening failed ({})", *gamePackage, msixFileResult.error());
					co_return false;
				}
				auto msixFile = *std::move(msixFileResult);

				auto result = co_await msixFile.ExtractAllFilesAsync(std::move(gameDirectory), progressCallback, std::move(contextFile));
				if (!result) {

					Logger::Error("Installing game package {} failed: extraction failed ({})", *gamePackage, result.error());
					co_return false;
				}
			}

			co_return true;
		}

		auto RevertGamePackageInstallationAsync(GamePackage const* gamePackage, ProgressToken progressToken) -> Task<> {

			using enum GamePackageStatus;

			progressToken(CancellingInstallation);
			co_await winrt::resume_background();

			auto gameDirectory = gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);

			auto ec = std::error_code{};
			if (std::filesystem::remove_all(gameDirectory, ec); ec) {

				Logger::Warn("Removing files for game package {} failed: {}", *gamePackage, ec.value());
			}
		}

		auto CleanupGamePackageInstallation(GamePackage const* gamePackage) -> void {

			auto gameDirectory = gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);
			auto contextFilePath = gameDirectory / ExtractionContextFileName;

			auto ec = std::error_code{};
			if (std::filesystem::remove(contextFilePath, ec); ec) {

				Logger::Warn("Removing extraction context file ({}) for game package {} failed: {}", contextFilePath, *gamePackage, ec.value());
			}
		}

		auto LaunchGamePackageAsync(GamePackage const* gamePackage, GameLaunchArgs launchArgs, ProgressToken progressToken) -> Task<bool> {

			using enum GamePackageStatus;

			Logger::Info("Launching game package {}", *gamePackage);

			if (gamePackage->Platform != GamePlatform::WindowsGDK && gamePackage->Platform != GamePlatform::WindowsUWP) {

				Logger::Error("Launching game package {} failed: unsupported platform", *gamePackage);
				co_return false;
			}

			progressToken(gamePackage->Platform == GamePlatform::WindowsGDK ? Launching : Registering);
			co_await winrt::resume_background();

			auto gameDirectory = gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);
			if (auto ec = std::error_code{}; !std::filesystem::is_directory(gameDirectory, ec)) {

				Logger::Error("Launching game package {} failed: game directory ({}) not found: {}", *gamePackage, gameDirectory, ec.value());
				co_return false;
			}

			auto manifestPath = gameDirectory / "AppxManifest.xml";
			auto manifest = Windows::MsixManifest::OpenFromFile(File{ manifestPath, FileMode::OpenExisting, FileAccess::Read });
			if (!manifest) {

				Logger::Error("Launching game package {} failed: package manifest opening failed ({})", *gamePackage, manifest.error());
				co_return false;
			}

			auto packageFamilyName = Windows::GetPackageFamilyNameFromId(manifest->Identity());
			auto& packageApp = manifest->Application();
			auto aumid = std::format("{}!{}", packageFamilyName, packageApp.Id);

			if (gamePackage->Platform == GamePlatform::WindowsGDK) {

				auto shortcutPath = gameDirectory / GetGameShortcutName(*gamePackage);
				if (auto result = Windows::Shell::CreateShortcut(shortcutPath, gameDirectory / packageApp.Executable, {}, aumid); !result) {

					Logger::Error("Launching game package {} failed: shortcut creation failed ({})", *gamePackage, result.error());
					co_return false;
				}

				auto args = std::string{};
				if (auto fileToImport = launchArgs.FileToImport(); !fileToImport.empty()) {

					args.push_back('"');
					ToUtf8(fileToImport, AppendTo(args));
					args.push_back('"');
				}
				else if (launchArgs.ActivateEditor()) {

					args = gamePackage->BuildType == GameBuildType::Preview
						? "minecraft-preview://creator/?Editor=true"
						: "minecraft://creator/?Editor=true";
				}

				if (auto result = co_await Windows::Shell::ExecuteAsync(shortcutPath, args); !result) {

					Logger::Error("Launching game package {} failed: process creation failed ({})", *gamePackage, result.error());
					co_return false;
				}
			}
			else {

				auto registeredPackage = winrt::Package{ nullptr };
				try {

					registeredPackage = GetRegisteredPackage(packageFamilyName);
				}
				catch (winrt::hresult_error const& error) {

					auto code = std::int32_t{ error.code() };
					auto message = error.message();

					Logger::Error(L"Launching game package {} failed: registered package retrieval failed (code: {}, message: {})", *gamePackage, code, message);
					co_return false;
				}

				auto alreadyRegistered = false;
				auto dataBackupDirectory = std::filesystem::path{};

				if (registeredPackage) try {

					auto packageFullName = registeredPackage.Id().FullName();
					auto packagePath = registeredPackage.InstalledPath();
					auto deploymentOperation = winrt::IAsyncOperationWithProgress<winrt::DeploymentResult, winrt::DeploymentProgress>{ nullptr };

					if (!registeredPackage.IsDevelopmentMode()) {

						auto dataBackupResult = BackupGameData(packageFamilyName, gamePackage->BuildType == GameBuildType::Preview ? "PreviewData" : "ReleaseData");
						if (!dataBackupResult) {

							Logger::Error("Launching game package {} failed: data backup creation failed", *gamePackage);
							co_return false;
						}
						dataBackupDirectory = *std::move(dataBackupResult);
						deploymentOperation = packageManager.RemovePackageAsync(packageFullName);
					}
					else if (auto ec = std::error_code{}; !std::filesystem::equivalent(std::wstring{ packagePath }, gameDirectory, ec)) {

						if (ec) {

							Logger::Error("Launching game package {} failed: filesystem error ({})", *gamePackage, ec.value());
							co_return false;
						}
						deploymentOperation = packageManager.RemovePackageAsync(packageFullName, winrt::RemovalOptions::PreserveApplicationData);
					}
					else {

						alreadyRegistered = true;
					}

					if (deploymentOperation) {

						Logger::Info(L"Removing registered package ({})", packageFullName);

						auto result = co_await deploymentOperation;
						if (deploymentOperation.Status() != winrt::AsyncStatus::Completed) {

							auto code = std::int32_t{ result.ExtendedErrorCode() };
							auto message = result.ErrorText();

							Logger::Error(L"Launching game package {} failed: removing registered package ({}) failed (code: {}, message: {})", *gamePackage, packageFullName, code, message);
							co_return false;
						}

						Logger::Info("Registered package removal completed");
					}
				}
				catch (winrt::hresult_error const& error) {

					auto code = std::int32_t{ error.code() };
					auto message = error.message();

					Logger::Error(L"Launching game package {} failed: registered package handling failure (code: {}, message: {})", *gamePackage, code, message);
					co_return false;
				}

				if (!alreadyRegistered) try {

					Logger::Info("Registering game package {}", *gamePackage);

					auto deploymentOpts = winrt::DeploymentOptions::DevelopmentMode | winrt::DeploymentOptions::ForceTargetApplicationShutdown;
					auto deploymentOperation = packageManager.RegisterPackageAsync(winrt::Uri{ manifestPath.native() }, nullptr, deploymentOpts);

					auto result = co_await deploymentOperation;
					if (deploymentOperation.Status() != winrt::AsyncStatus::Completed) {

						auto code = std::int32_t{ result.ExtendedErrorCode() };
						auto message = result.ErrorText();

						Logger::Error(L"Launching game package {} failed: registration failed (code: {}, message: {})", *gamePackage, code, message);
						co_return false;
					}

					Logger::Info("Registration of game package {} completed", *gamePackage);

					if (!dataBackupDirectory.empty() && !RestoreGameData(packageFamilyName, dataBackupDirectory)) {

						Logger::Error("Launching game package {} failed: data restoration failed", *gamePackage);
						co_return false;
					}
				}
				catch (winrt::hresult_error const& error) {

					auto code = std::int32_t{ error.code() };
					auto message = error.message();

					Logger::Error(L"Launching game package {} failed: registration failed (code: {}, message: {})", *gamePackage, code, message);
					co_return false;
				}

				progressToken(Launching);

				Windows::AppLauncher::EnableDebugging(manifest->Identity().FullName());

				auto launchTask = Windows::AsyncAppLaunchResult{ nullptr };
				if (auto fileToImport = launchArgs.FileToImport(); !fileToImport.empty()) {

					launchTask = Windows::AppLauncher::LaunchForFileAsync(aumid, std::wstring{ fileToImport });
				}
				else {

					launchTask = Windows::AppLauncher::LaunchAsync(aumid, launchArgs.ActivateEditor() ? "Editor=true" : "");
				}

				if (auto result = co_await std::move(launchTask); !result) {

					Logger::Error("Launching game package {} failed: process creation failed ({})", *gamePackage, result.error());
					co_return false;
				}
			}

			co_return true;
		}

		auto UninstallGamePackageAsync(GamePackage const* gamePackage, ProgressToken progressToken) -> Task<bool> {

			using enum GamePackageStatus;

			progressToken(Uninstalling);
			co_await winrt::resume_background();

			auto gameDirectory = gamePackage->InstallLocation / GetGameDirectoryName(*gamePackage);
			auto packageFamilyName = std::string{};
			auto targetExecutableName = std::filesystem::path{};

			{
				auto manifest = Windows::MsixManifest::OpenFromFile(File{ gameDirectory / L"AppxManifest.xml", FileMode::OpenExisting, FileAccess::Read });
				if (manifest) {

					packageFamilyName = Windows::GetPackageFamilyNameFromId(manifest->Identity());
					targetExecutableName = manifest->Application().Executable;
				}
			}

			if (gamePackage->Platform == GamePlatform::WindowsGDK) {

				auto snapshot = wil::unique_tool_help_snapshot{};
				if (targetExecutableName.has_filename()) {

					snapshot = wil::unique_tool_help_snapshot{ ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
					if (!snapshot) {

						Logger::Warn("Retrieving process snapshot failed: {}", ::GetLastError());
					}
				}

				auto snapshotEntry = ::PROCESSENTRY32W{ .dwSize = sizeof(::PROCESSENTRY32W) };
				if (snapshot && ::Process32FirstW(snapshot.get(), &snapshotEntry)) {

					auto targetExecutablePath = gameDirectory / targetExecutableName;
					auto targetProcesses = std::vector<wil::unique_process_handle>{};
					auto targetPids = std::vector<::DWORD>{};

					do {

						if (std::wstring_view{ snapshotEntry.szExeFile } != targetExecutableName)
							continue;

						auto pid = snapshotEntry.th32ProcessID;
						auto process = wil::unique_process_handle{ ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, false, pid) };
						if (!process)
							continue;

						auto pathBuffer = TrivialArray<wchar_t, MAX_PATH>{};
						auto pathSize = ::DWORD{ pathBuffer.size() };
						if (!QueryFullProcessImageNameW(process.get(), 0, pathBuffer.data(), &pathSize))
							continue;

						auto executablePath = std::filesystem::path{ std::wstring{ pathBuffer.data(), pathSize } };
						if (auto ec = std::error_code{}; !std::filesystem::equivalent(executablePath, targetExecutablePath))
							continue;

						targetProcesses.emplace_back(std::move(process));
						targetPids.emplace_back(pid);

					} while (Process32NextW(snapshot.get(), &snapshotEntry));
					snapshot.reset();

					auto enumWindowsCallback = [](::HWND hwnd, ::LPARAM lParam) static -> BOOL CALLBACK {

						auto targetPids = std::bit_cast<std::vector<::DWORD>*>(lParam);
						auto pid = ::DWORD{};
						::GetWindowThreadProcessId(hwnd, &pid);

						if (std::ranges::contains(*targetPids, pid))
							::PostMessageW(hwnd, WM_CLOSE, 0, 0);
						return true;
					};
					::EnumWindows(enumWindowsCallback, std::bit_cast<::LPARAM>(&targetPids));

					for (auto [targetProcess, pid] : std::views::zip(targetProcesses, targetPids)) {

						try {

							co_await winrt::resume_on_signal(targetProcess.get(), 15s);
							targetProcess.reset();
						}
						catch (winrt::hresult_error const& error) {

							auto code = std::int32_t{ error.code() };
							auto message = error.message();

							Logger::Error(L"Uninstalling game package {} failed: game process closure failed (code: {}, message: {}), pid = {}", *gamePackage, code, message, pid);
							co_return false;
						}
					}
				}

				auto delay = 50ms;
				for (auto attempts = 5; attempts > 0; --attempts) {

					co_await winrt::resume_after(delay);

					auto ec = std::error_code{};
					if (std::filesystem::remove_all(gameDirectory, ec); ec) {

						if (ec.value() == ERROR_SHARING_VIOLATION && attempts > 1) {

							delay *= 2;
							continue;
						}

						Logger::Error("Uninstalling game package {} failed: files removal failed ({})", *gamePackage, ec.value());
						co_return false;
					}
					break;
				}

				auto ec = std::error_code{};
				if (std::filesystem::remove_all(gameDirectory, ec); ec) {

					Logger::Error("Uninstalling game package {} failed: files removal failed ({})", *gamePackage, ec.value());
					co_return false;
				}
			}
			else if (gamePackage->Platform == GamePlatform::WindowsUWP) {

				auto registeredPackage = winrt::Package{ nullptr };
				if (!packageFamilyName.empty()) try {

					registeredPackage = GetRegisteredPackage(packageFamilyName);
				}
				catch (winrt::hresult_error const& error) {

					auto code = std::int32_t{ error.code() };
					auto message = error.message();

					Logger::Error(L"Uninstalling game package {} failed: registered package retrieval failed (code: {}, message: {})", *gamePackage, code, message);
					co_return false;
				}

				if (registeredPackage) try {

					auto& deploymentMutex = GetDeploymentMutex(*gamePackage);
					auto deploymentMutexAcquired = false;
					auto releaseDeploymentMutex = ScopeExit{ [&] { if (deploymentMutexAcquired) deploymentMutex.release(); } };

					auto ec = std::error_code{};
					if (std::filesystem::equivalent(std::wstring{ registeredPackage.InstalledPath() }, gameDirectory, ec)) {

						deploymentMutexAcquired = deploymentMutex.try_acquire();
						if (!deploymentMutexAcquired) {

							progressToken(UninstallationPending);
							deploymentMutex.acquire();
							deploymentMutexAcquired = true;
							progressToken(Uninstalling);
						}

						registeredPackage = GetRegisteredPackage(packageFamilyName);
					}
					else if (ec) {

						Logger::Error("Uninstalling game package {} failed: filesystem error ({})", *gamePackage, ec.value());
						co_return false;
					}

					if (deploymentMutexAcquired && registeredPackage && std::filesystem::equivalent(std::wstring{ registeredPackage.InstalledPath() }, gameDirectory, ec)) {

						auto packageFullName = registeredPackage.Id().FullName();
						Logger::Info(L"Removing registered package ({})", packageFullName);

						auto deploymentOperation = packageManager.RemovePackageAsync(packageFullName, winrt::RemovalOptions::PreserveApplicationData);
						auto result = co_await deploymentOperation;
						if (deploymentOperation.Status() != winrt::AsyncStatus::Completed) {

							auto code = std::int32_t{ result.ExtendedErrorCode() };
							auto message = result.ErrorText();

							Logger::Error(L"Uninstalling game package {} failed: removing registered package ({}) failed (code: {}, message: {})", *gamePackage, packageFullName, code, message);
							co_return false;
						}

						Logger::Info("Registered package removal completed");
					}
					else if (ec) {

						Logger::Error("Uninstalling game package {} failed: filesystem error ({})", *gamePackage, ec.value());
						co_return false;
					}
				}
				catch (winrt::hresult_error const& error) {

					auto code = std::int32_t{ error.code() };
					auto message = error.message();

					Logger::Error(L"Uninstalling game package {} failed: registered package handling failure (code: {}, message: {})", *gamePackage, code, message);
					co_return false;
				}

				auto ec = std::error_code{};
				if (std::filesystem::remove_all(gameDirectory, ec); ec) {

					if (ec.value() != ERROR_SHARING_VIOLATION) {

						Logger::Error("Uninstalling game package {} failed: files removal failed ({})", *gamePackage, ec.value());
						co_return false;
					}

					progressToken(UninstallationPending);
					auto& deploymentMutex = GetDeploymentMutex(*gamePackage);
					deploymentMutex.acquire();
					deploymentMutex.release();
					progressToken(Uninstalling);

					if (std::filesystem::remove_all(gameDirectory, ec); ec) {

						Logger::Error("Uninstalling game package {} failed: files removal failed ({})", *gamePackage, ec.value());
						co_return false;
					}
				}
			}
			else {

				Logger::Warn("Removal of game package {} from system not handled", *gamePackage);
			}

			co_return true;
		}

		auto BackupGameData(std::string_view packageFamilyName, std::string_view backupDirName) -> std::optional<std::filesystem::path> {

			Logger::Info("Creating data backup for package family {}", packageFamilyName);

			auto calculatePath = [path = backupsDirectory / backupDirName](std::size_t index) -> std::filesystem::path {

				if (index == 0)
					return path;
				return std::format(L"{}.{}", path.native(), index);
			};

			for (auto i = 0uz; i < 0x100; ++i) {

				auto backupDirectory = calculatePath(i);
				if (auto ec = std::error_code{}; std::filesystem::exists(backupDirectory, ec))
					continue;

				auto localDataDirectory = std::filesystem::path{};
				try {

					auto packageAppData = winrt::ApplicationDataManager::CreateForPackageFamily(winrt::to_hstring(packageFamilyName));
					localDataDirectory = std::wstring{ packageAppData.LocalFolder().Path() };
				}
				catch (winrt::hresult_error const& error) {
				
					auto code = std::int32_t{ error.code() };
					auto message = ToUtf8(error.message());

					Logger::Error("Creating data backup for package family {} failed: data directory retrieval failed (code: {}, message: {})", packageFamilyName, code, message);
					return std::nullopt;
				}

				auto ec = std::error_code{};
				if (std::filesystem::rename(localDataDirectory, backupDirectory, ec); ec) {

					Logger::Error("Creating data backup for package family {} failed: moving data to backup directory ({}) failed ({})", packageFamilyName, backupDirectory, ec.value());
					return std::nullopt;
				}

				Logger::Info("Data backup creation for package family {} completed, backup directory: {}", packageFamilyName, backupDirectory);
				return backupDirectory;
			}

			Logger::Error("Creating data backup for package family {} failed: backup limit exceeded", packageFamilyName);
			return std::nullopt;
		}

		auto RestoreGameData(std::string_view packageFamilyName, std::filesystem::path const& backupDirPath) -> bool {

			Logger::Info("Restoring data for package family {}, data backup: {}", packageFamilyName, backupDirPath);

			auto localDataDirectory = std::filesystem::path{};
			try {

				auto packageAppData = winrt::ApplicationDataManager::CreateForPackageFamily(winrt::to_hstring(packageFamilyName));
				localDataDirectory = std::wstring{ packageAppData.LocalFolder().Path() };
			}
			catch (winrt::hresult_error const& error) {

				auto code = std::int32_t{ error.code() };
				auto message = ToUtf8(error.message());

				Logger::Error("Restoring data for package family {} failed: data directory retrieval failed (code: {}, message: {})", packageFamilyName, code, message);
				return false;
			}

			auto ec = std::error_code{};
			if (std::filesystem::is_directory(localDataDirectory, ec) && !std::filesystem::is_empty(localDataDirectory, ec)) {

				Logger::Error("Restoring data for package family {} failed: data directory not empty", packageFamilyName);
				return false;
			}

			if (ec) {

				Logger::Error("Restoring data for package family {} failed: filesystem error ({})", packageFamilyName, ec.value());
				return false;
			}

			if (std::filesystem::remove(localDataDirectory, ec); ec) {

				Logger::Error("Restoring data for package family {} failed: data directory removal failed ({})", packageFamilyName, ec.value());
				return false;
			}

			if (std::filesystem::rename(backupDirPath, localDataDirectory, ec); ec) {

				Logger::Error("Restoring data for package family {} failed: moving backup data ({}) to data directory failed ({})", packageFamilyName, backupDirPath, ec.value());
				return false;
			}

			Logger::Info("Data restoration for package family {} completed", packageFamilyName);
			return true;
		}

		auto GetRegisteredPackage(std::string_view packageFamilyName) -> winrt::Package {

			auto& currentUser = Windows::GetCurrentUser();
			auto wPackageFamilyName = winrt::to_hstring(packageFamilyName);

			auto packages = packageManager.FindPackagesForUser(winrt::to_hstring(currentUser.Sid), wPackageFamilyName);
			auto registeredPackage = winrt::Package{ nullptr };

			auto it = packages.First();
			if (it.HasCurrent()) {

				registeredPackage = it.Current();
				it.MoveNext();
			}

			if (it.HasCurrent()) 
				throw winrt::hresult_error{ E_UNEXPECTED, winrt::format(L"Unsupported scenario: more than one registered package found for package family {}", wPackageFamilyName) };
			return registeredPackage;
		}

		auto GetDeploymentMutex(GamePackageIdentity const& packageId) -> std::binary_semaphore& {

			return packageId.BuildType == GameBuildType::Preview
				? previewBuildDeploymentMutex
				: releaseBuildDeploymentMutex;
		}

		struct SettingsT : public MinecraftBedrockGameManagerSettings {

			friend GameManagerInternal;
		};

		bool initialized{};
		std::filesystem::path rootDirectory;
		std::filesystem::path backupsDirectory;
		SettingsT settings;
		GamePackageMeta gameMeta;
		ServerPackageMeta serverMeta;
		JsonStorage<GamePackageOperationCollection> gamePackageOperations;
		JsonStorage<GamePackageCollection> gameInstallations;
		winrt::com_ptr<ObservableCollection<GamePackageItem>> releaseGamePackages = winrt::make_self<ObservableCollection<GamePackageItem>>();
		winrt::com_ptr<ObservableCollection<GamePackageItem>> previewGamePackages = winrt::make_self<ObservableCollection<GamePackageItem>>();
		winrt::com_ptr<ObservableCollection<GamePackageItem>> importedGamePackages = winrt::make_self<ObservableCollection<GamePackageItem>>();

		File releaseBuildProgressionsFile;
		File previewBuildProgressionsFile;

		winrt::PackageManager packageManager;
		std::binary_semaphore releaseBuildDeploymentMutex{ 1 };
		std::binary_semaphore previewBuildDeploymentMutex{ 1 };

		ProgressDispatcher progressDispatcher;
		Event<EventHandler<>> initializationCompletedEvent;
	};

	GameManagerInternal gameManagerInternal;
}

namespace Citrine {

	auto MinecraftBedrockGameManager::InitializeAsync() -> void {

		gameManagerInternal.InitializeAsync();
	}

	auto MinecraftBedrockGameManager::InitializationCompleted(EventHandler<> handler) -> EventToken {

		return gameManagerInternal.InitializationCompleted(std::move(handler));
	}

	auto MinecraftBedrockGameManager::InitializationCompleted(EventToken&& token) -> void {

		gameManagerInternal.InitializationCompleted(std::move(token));
	}

	auto MinecraftBedrockGameManager::Settings() -> MinecraftBedrockGameManagerSettings& {

		return gameManagerInternal.Settings();
	}

	auto MinecraftBedrockGameManager::ReleaseGamePackages() -> winrt::Citrine::IObservableCollectionView {

		return gameManagerInternal.ReleaseGamePackages();
	}

	auto MinecraftBedrockGameManager::PreviewGamePackages() -> winrt::Citrine::IObservableCollectionView {
	
		return gameManagerInternal.PreviewGamePackages();
	}

	auto MinecraftBedrockGameManager::ImportedGamePackages() -> winrt::Citrine::IObservableCollectionView {

		return gameManagerInternal.ImportedGamePackages();
	}

	auto MinecraftBedrockGameManager::ValidateInstallLocation(std::filesystem::path const& path) -> winrt::Citrine::InstallLocationValidationResult {

		return gameManagerInternal.ValidateInstallLocation(path);
	}

	auto MinecraftBedrockGameManager::InstallGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void {

		gameManagerInternal.InstallGamePackageAsync(std::move(item));
	}

	auto MinecraftBedrockGameManager::InitiateGamePackageImportAsync(std::filesystem::path gamePackageLocation) -> Task<winrt::Citrine::MinecraftBedrockGamePackageImportContext> {

		return gameManagerInternal.InitiateGamePackageImportAsync(std::move(gamePackageLocation));
	}

	auto MinecraftBedrockGameManager::ImportGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageImportContext importContext, winrt::hstring nameTag) -> void {

		return gameManagerInternal.ImportGamePackageAsync(std::move(importContext), std::move(nameTag));
	}

	auto MinecraftBedrockGameManager::LaunchGamePackageAsync(winrt::Citrine::MinecraftBedrockGameLaunchArgs launchArgs) -> Task<winrt::Citrine::MinecraftBedrockGameLaunchResult> {

		return gameManagerInternal.LaunchGamePackageAsync(std::move(launchArgs));
	}

	auto MinecraftBedrockGameManager::RenameGamePackage(winrt::Citrine::MinecraftBedrockGamePackageItem const& item, winrt::hstring const& nameTag) -> void {

		gameManagerInternal.RenameGamePackage(item, nameTag);
	}

	auto MinecraftBedrockGameManager::UninstallGamePackageAsync(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void {

		gameManagerInternal.UninstallGamePackageAsync(std::move(item));
	}

	auto MinecraftBedrockGameManager::GetGameDirectory(winrt::Citrine::MinecraftBedrockGamePackageItem const& item) -> std::filesystem::path {

		return gameManagerInternal.GetGameDirectory(item);
	}

	auto MinecraftBedrockGameManager::GetGameDataDirectory(winrt::Citrine::MinecraftBedrockGamePackageItem const& item) -> std::filesystem::path {

		return gameManagerInternal.GetGameDataDirectory(item);
	}

	auto MinecraftBedrockGameManager::PauseGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void {

		gameManagerInternal.PauseGamePackageOperation(std::move(item));
	}

	auto MinecraftBedrockGameManager::ResumeGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void {

		gameManagerInternal.ResumeGamePackageOperation(std::move(item));
	}

	auto MinecraftBedrockGameManager::CancelGamePackageOperation(winrt::Citrine::MinecraftBedrockGamePackageItem item) -> void {

		gameManagerInternal.CancelGamePackageOperation(std::move(item));
	}

	auto MinecraftBedrockGameManager::Shutdown() -> void {

		gameManagerInternal.Shutdown();
	}

	auto MinecraftBedrockGameManagerSettings::GameInstallLocation() const noexcept -> std::filesystem::path const& {

		return storage->GameInstallLocation;
	}

	auto MinecraftBedrockGameManagerSettings::GameInstallLocation(std::filesystem::path value) noexcept -> void {

		storage->GameInstallLocation = std::move(value);
	}

	auto MinecraftBedrockGameManagerSettings::Open(std::filesystem::path const& path) -> bool {

		return storage.Open(path);
	}

	auto MinecraftBedrockGameManagerSettings::Close() -> void {

		storage.Close();
	}

	auto MinecraftBedrockGameManagerSettings::Load() -> StorageOperationResult {

		return storage.Load();
	}

	auto MinecraftBedrockGameManagerSettings::Save() -> StorageOperationResult {

		return storage.Save();
	}
}