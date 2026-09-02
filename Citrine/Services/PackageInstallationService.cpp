#include "pch.h"
#include "PackageInstallationService.h"

#include "ServiceImplBase.h"

#include "Core/Logging/Logger.h"
#include "Core/Util/Ascii.h"
#include "Core/Net/UrlQuery.h"
#include "Windows/FE3Handler.h"
#include "Windows/User.h"
#include "Services/HttpService.h"

#include <vector>
#include <shared_mutex>
#include <type_traits>
#include <algorithm>

#include <winrt/Windows.Management.Deployment.h>

#include <glaze/json.hpp>

using namespace Citrine;
using namespace Windows;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Management::Deployment;
}

namespace {

#if defined(_M_X64)
	
	constexpr auto DefaultTargetArchitecture = PackageArchitecture::X64;
	constexpr auto FrameworkTargetArchitectures = { PackageArchitecture::X64, PackageArchitecture::X86 };

#endif

	class PackageInstallationServiceInternal : ServiceImplBase {
	public:

		static constexpr auto& Name = "PackageInstallationService";

		class FE3CookieFetchOperation : public Operation<std::shared_ptr<FE3Cookie const>> {
		public:

			FE3CookieFetchOperation() = default;
		};

		class WUCategoryIdFetchOperation : public Operation<Guid> {
		public:

			WUCategoryIdFetchOperation(StringParameter packageFamilyName)

				: PackageFamilyName(std::move(packageFamilyName))
			{}

			std::string const PackageFamilyName;
		};

		class PackageInstallOperation : public Operation<bool> {
		public:

			PackageInstallOperation(StringParameter packageFamilyName, PackageArchitecture targetArchitecture)

				: PackageFamilyName(std::move(packageFamilyName))
				, TargetArchitecture(targetArchitecture)
			{}

			std::string const PackageFamilyName;
			PackageArchitecture const TargetArchitecture;
		};

		PackageInstallationServiceInternal() {

			RegisterAction<InstallPackageAction>();
		}

		auto GetFE3CookieAsync() -> Task<std::shared_ptr<FE3Cookie const>> {

			auto opHandle = OperationHandle<FE3CookieFetchOperation>{};
			{
				auto operations = activeOperations.Lock();
				for (auto& operation : operations) {

					auto op = dynamic_cast<FE3CookieFetchOperation*>(operation);
					if (!op)
						continue;

					opHandle.Attach(op);
					break;
				}

				if (!opHandle) {

					auto task = InvokeGetFE3Cookie();
					auto op = operations.Add(std::make_unique<FE3CookieFetchOperation>(), std::move(task));
					opHandle.Attach(op);
				}
			}

			co_return co_await opHandle;
		}

		auto GetWUCategoryIdAsync(StringParameter packageFamilyName) -> Task<Guid> {

			auto opHandle = OperationHandle<WUCategoryIdFetchOperation>{};
			{
				auto operations = activeOperations.Lock();
				for (auto& operation : operations) {

					auto op = dynamic_cast<WUCategoryIdFetchOperation*>(operation);
					if (!op)
						continue;

					if (!Ascii::CaseInsensitiveEquals(op->PackageFamilyName, packageFamilyName.View()))
						continue;

					opHandle.Attach(op);
					break;
				}

				if (!opHandle) {

					auto task = InvokeGetWUCategoryId(packageFamilyName);
					auto op = operations.Add(std::make_unique<WUCategoryIdFetchOperation>(std::move(packageFamilyName)), std::move(task));
					opHandle.Attach(op);
				}
			}

			co_return co_await opHandle;
		}

		auto InstallPackageAsync(StringParameter packageFamilyName, std::shared_ptr<FE3UpdateInfo const> updateInfo = nullptr) -> Task<bool> {

			auto targetArchitecture = updateInfo
				? updateInfo->PackageMetadata.PackageId.Architecture()
				: DefaultTargetArchitecture;

			auto opHandle = OperationHandle<PackageInstallOperation>{};
			{
				auto operations = activeOperations.Lock();
				for (auto& operation : operations) {

					auto op = dynamic_cast<PackageInstallOperation*>(operation);
					if (!op)
						continue;

					if (!Ascii::CaseInsensitiveEquals(op->PackageFamilyName, packageFamilyName.View()) || op->TargetArchitecture != targetArchitecture)
						continue;

					opHandle.Attach(op);
					break;
				}

				if (!opHandle) {

					auto task = InvokeInstallPackage(packageFamilyName, std::move(updateInfo));
					auto op = operations.Add(std::make_unique<PackageInstallOperation>(std::move(packageFamilyName), targetArchitecture), std::move(task));
					opHandle.Attach(op);
				}
			}

			co_return co_await opHandle;
		}

	private:

		struct PackageInstallArgs {

			Url Url;

			struct glaze {

				using T = PackageInstallArgs;
				static constexpr auto value = glz::object(
					"Url", &T::Url
				);
			};
		};

		struct PackageInstallResult {

			std::int32_t Code{};
			std::string Message;

			struct glaze {

				using T = PackageInstallResult;
				static constexpr auto value = glz::object(
					"Code", &T::Code,
					"Message", &T::Message
				);
			};
		};

		struct InstallPackageAction {

			static constexpr auto Name = "InstallPackage";

			static auto Execute(PackageInstallArgs args) -> LazyTask<PackageInstallResult> try {
				
				auto& url = args.Url;
				auto packageManager = winrt::PackageManager{};

				auto addPackageOpts = winrt::AddPackageOptions{};
				auto deploymentOperation = packageManager.AddPackageByUriAsync(winrt::Uri{ winrt::to_hstring(url.RawUrl()) }, addPackageOpts);

				auto result = co_await deploymentOperation;
				if (deploymentOperation.Status() != winrt::AsyncStatus::Completed) {

					auto code = std::int32_t{ result.ExtendedErrorCode() };
					auto message = winrt::to_string(result.ErrorText());

					co_return { code, std::move(message) };
				}

				co_return { S_OK };
			}
			catch (winrt::hresult_error const& error) {

				auto code = std::int32_t{ error.code() };
				auto message = winrt::to_string(error.message());

				co_return{ code, std::move(message) };
			}
		};

		auto InvokeGetFE3Cookie() -> OperationTask<std::shared_ptr<FE3Cookie const>> {

			{
				auto lock = std::shared_lock{ mutex };
				if (cachedFE3Cookie && cachedFE3Cookie->IsValid())
					co_return cachedFE3Cookie;
			}

			co_await winrt::resume_background();

			Logger::Info("Fetching FE3Cookie");

			auto result = co_await Windows::FE3Handler::GetCookieAsync();
			if (!result) {

				Logger::Error("Fetching FE3Cookie failed: {}", result.error());
				co_return nullptr;
			}

			Logger::Info("Fetching FE3Cookie completed");

			{
				auto lock = std::unique_lock{ mutex };
				cachedFE3Cookie = std::make_shared<Windows::FE3Cookie>(std::move(*result));
				co_return cachedFE3Cookie;
			}
		}

		auto InvokeGetWUCategoryId(std::string packageFamilyName) -> OperationTask<Guid> {

			co_await winrt::resume_background();

			Logger::Info("Fetching WUCategoryId for package family {}", packageFamilyName);

			auto query = UrlQuery{

				{ "market", "neutral" },
				{ "languages", "neutral" },
				{ "fieldsTemplate", "installAgent" },
				{ "alternateId", "PackageFamilyName" },
				{ "value", packageFamilyName }
			};

			auto rawUrl = std::string{ "https://displaycatalog.mp.microsoft.com/v7.0/products/lookup" };
			rawUrl.push_back('?');
			query.Serialize(AppendTo(rawUrl));

			auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, std::move(rawUrl));
			if (!responseMessage) {

				Logger::Error("Fetching WUCategoryId for package family {} failed: network error", packageFamilyName);
				co_return {};
			}

			if (!responseMessage->IsSuccessful()) {

				Logger::Error("Fetching WUCategoryId for package family {} failed: api error", packageFamilyName);
				co_return {};
			}

			auto wuCategoryId = Guid{};
			constexpr auto& path = "Products[0].DisplaySkuAvailabilities[0].Sku.Properties.FulfillmentData.WuCategoryId";

			constexpr auto opts = glz::opts{ .null_terminated = false };
			if (auto ec = glz::read_jmespath<{ path }, opts>(wuCategoryId, responseMessage->Content); ec) {

				Logger::Error("Fetching WUCategoryId for package family {} failed: response error", packageFamilyName);
				co_return {};
			}

			Logger::Info("Fetching WUCategoryId for package family {} completed ({})", packageFamilyName, wuCategoryId);
			co_return wuCategoryId;
		}

		auto InvokeInstallPackage(std::string packageFamilyName, std::shared_ptr<FE3UpdateInfo const> updateInfo) -> OperationTask<bool> {

			co_await winrt::resume_background();

			auto targetArchitecture = PackageArchitecture{};

			if (!updateInfo) {

				targetArchitecture = DefaultTargetArchitecture;

				Logger::Info("Initiating install of package {}", packageFamilyName);

				auto wuCategoryId = co_await GetWUCategoryIdAsync(packageFamilyName);
				if (wuCategoryId.IsEmpty()) {

					Logger::Error("Installing package {} failed: WUCategoryId fetching failed", packageFamilyName);
					co_return false;
				}

				auto fe3Cookie = co_await GetFE3CookieAsync();
				if (!fe3Cookie) {

					Logger::Error("Installing package {} failed: FE3Cookie fetching failed", packageFamilyName);
					co_return false;
				}

				auto updatesResult = co_await FE3Handler::GetUpdatesAsync(wuCategoryId, *fe3Cookie);
				if (!updatesResult) {

					Logger::Error("Installing package {} failed: update fetching failed", packageFamilyName);
					co_return false;
				}

				auto updates = std::make_shared<std::vector<FE3UpdateInfo> const>(*std::move(updatesResult));
				auto filteredUpdates = std::vector<FE3UpdateInfo const*>{};
				filteredUpdates.reserve(8);

				auto checkPlatform = [&deviceFamilyInfo = DeviceFamilyInfo::Get()](FE3UpdateInfo const& updateInfo) -> bool {

					auto& packageMetadata = updateInfo.PackageMetadata;

					for (auto const& targetPlatform : packageMetadata.TargetPlatforms) {

						auto const& [deviceFamily, minVersion] = targetPlatform;
						if ((deviceFamily == DeviceFamily::Universal || deviceFamily == deviceFamilyInfo.DeviceFamily) && minVersion <= deviceFamilyInfo.DeviceFamilyVersion)
							return true;
					}
					return false;
				};

				auto checkArchitecture = [](FE3UpdateInfo const& updateInfo) static -> bool {

					auto& packageMetadata = updateInfo.PackageMetadata;
					
					if (packageMetadata.IsFramework) {

						auto& packageId = packageMetadata.PackageId;
						auto architecture = packageId.Architecture();

						for (auto targetArchitecture : FrameworkTargetArchitectures) {

							if (architecture == targetArchitecture)
								return true;
						}
					}
					else {

						auto packages = packageMetadata.IsBundle ? std::span{ packageMetadata.BundledPackages } : std::span{ &packageMetadata.PackageId, 1 };

						for (auto const& package : packageMetadata.BundledPackages) {

							auto& packageId = package;
							auto architecture = packageId.Architecture();

							if (architecture == DefaultTargetArchitecture)
								return true;
						}
					}
					return false;
				};

				for (auto const& update : *updates) {

					if (!checkPlatform(update))
						continue;

					if (!checkArchitecture(update))
						continue;

					auto it = std::ranges::find_if(filteredUpdates, [&update](FE3UpdateInfo const* otherUpdate) {
						
						auto packageId = PackageIdentityView{ update.PackageMetadata.PackageId };
						auto otherPackageId = PackageIdentityView{ otherUpdate->PackageMetadata.PackageId };

						return
							Ascii::CaseInsensitiveEquals(packageId.Name(), otherPackageId.Name()) &&
							Ascii::CaseInsensitiveEquals(packageId.PublisherId(), otherPackageId.PublisherId()) &&
							packageId.Architecture() == otherPackageId.Architecture();
					});

					if (it != filteredUpdates.end()) {

						auto version = update.PackageMetadata.PackageId.Version();
						auto otherVersion = (*it)->PackageMetadata.PackageId.Version();

						if (version > otherVersion)
							*it = &update;
					}
					else {

						filteredUpdates.emplace_back(&update);
					}
				}

				auto makeShared = [&updates](FE3UpdateInfo const* update) {

					return std::shared_ptr<FE3UpdateInfo const>{ updates, update };
				};

				for (auto const* update : filteredUpdates) {

					auto& packageMetadata = update->PackageMetadata;
					auto& packageId = packageMetadata.PackageId;

					auto familyName = GetPackageFamilyNameFromId(packageMetadata.PackageId);
					auto architecture = packageId.Architecture();

					if (packageMetadata.IsFramework) {

						if (Ascii::CaseInsensitiveEquals(familyName, packageFamilyName) && architecture == targetArchitecture) {

							updateInfo = makeShared(update);
						}
						else if (!co_await InstallPackageAsync(std::move(familyName), makeShared(update))) {

							Logger::Error("Installing package {} failed: dependency installation failed", packageFamilyName);
							co_return false;
						}
					}
					else {

						if (Ascii::CaseInsensitiveEquals(familyName, packageFamilyName)) {
							
							updateInfo = makeShared(update);
						}
						else {

							Logger::Error("Installing package {} failed: mismatching app package found ({})", packageFamilyName, familyName);
							co_return false;
						}
					}
				}
			}
			else {

				targetArchitecture = updateInfo->PackageMetadata.PackageId.Architecture();
			}

			if (!updateInfo) {

				Logger::Error("Installing package {} failed: not found", packageFamilyName);
				co_return false;
			}

			try {

				auto& packageMetadata = updateInfo->PackageMetadata;
				auto selectedPackageId = static_cast<PackageIdentity const*>(nullptr);

				if (packageMetadata.IsBundle) {

					for (auto const& package : packageMetadata.BundledPackages) {

						auto& packageId = package;
						auto architecture = packageId.Architecture();

						if (architecture == targetArchitecture) {

							selectedPackageId = &packageId;
							break;
						}
					}
				}
				else {

					auto& packageId = packageMetadata.PackageId;

					selectedPackageId = &packageId;
				}

				if (!selectedPackageId) {

					Logger::Error("Installing package {} failed: no package with target architecture {} found", packageFamilyName, targetArchitecture);
					co_return false;
				}

				auto latestVersion = selectedPackageId->Version();
				auto& currentUser = Windows::GetCurrentUser();

				for (auto const& package : packageManager.FindPackagesForUser(winrt::to_hstring(currentUser.Sid), winrt::to_hstring(packageFamilyName))) {

					auto packageId = package.Id();
					auto architecture = PackageArchitecture{ static_cast<std::uint16_t>(packageId.Architecture()) };
					auto version = std::bit_cast<PackageVersion>(packageId.Version());

					if (architecture == targetArchitecture && version >= latestVersion) {

						Logger::Info("The latest version of package {} with target architecture {} is already installed", packageFamilyName, targetArchitecture);
						co_return true;
					}
				}
			}
			catch (winrt::hresult_error const& error) {

				auto code = std::int32_t{ error.code() };
				auto message = winrt::to_string(error.message());

				Logger::Error("Installing package {} failed: installed package retrieval failed (code: {}, message: {})", packageFamilyName, code, message);
				co_return false;
			}

			auto& packageId = updateInfo->PackageMetadata.PackageId;
			Logger::Info("Installing package {}", packageId);

			auto url = Url{};
			auto retryWithAdminPrivileges = false;

			try {

				auto& [updateId, revisionNumber] = updateInfo->UpdateIdentity;

				auto urlResult = co_await FE3Handler::GetFileUrlAsync(updateId, revisionNumber);
				if (!urlResult) {

					Logger::Error("Installing package {} failed: url fetching failed", packageId);
					co_return false;
				}
				url = std::move(*urlResult);

				auto addPackageOpts = winrt::AddPackageOptions{};
				auto deploymentOperation = packageManager.AddPackageByUriAsync(winrt::Uri{ winrt::to_hstring(url.RawUrl()) }, addPackageOpts);

				auto result = co_await deploymentOperation;
				if (deploymentOperation.Status() != winrt::AsyncStatus::Completed) {

					auto code = std::int32_t{ result.ExtendedErrorCode() };
					auto message = winrt::to_string(result.ErrorText());

					if (code == 0x8A150019 || code == 0x80073D28) {

						retryWithAdminPrivileges = true;
					}
					else {

						Logger::Error("Installing package {} failed (code: {}, message: {})", packageId, code, message);
						co_return false;
					}
				}
			}
			catch (winrt::hresult_error const& error) {

				auto code = std::int32_t{ error.code() };
				auto message = winrt::to_string(error.message());

				Logger::Error("Installing package {} failed (code: {}, message: {})", packageId, code, message);
				co_return false;
			}

			if (retryWithAdminPrivileges) {

				auto args = PackageInstallArgs{ std::move(url) };
				auto result = PackageInstallResult{};

				if (!co_await RunActionInAdminContext<InstallPackageAction>(args, result)) {

					Logger::Error("Installing package {} failed: elevated action invocation failed", packageId);
					co_return false;
				}

				auto& [code, message] = result;

				if (code != S_OK) {

					Logger::Error("Installing package {} failed (code: {}, message: {})", packageId, code, message);
					co_return false;
				}
			}

			Logger::Info("Installation of package {} completed", packageId);
			co_return true;
		}

		OperationCollection activeOperations;
		winrt::PackageManager packageManager;

		std::shared_mutex mutex;
		std::shared_ptr<FE3Cookie const> cachedFE3Cookie;
	};

	PackageInstallationServiceInternal packageInstallationServiceInternal;
}

namespace Citrine {

	auto PackageInstallationService::InstallPackageAsync(StringParameter packageFamilyName) -> Task<bool> {

		return packageInstallationServiceInternal.InstallPackageAsync(std::move(packageFamilyName));
	}
}