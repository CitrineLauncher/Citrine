#include "pch.h"
#include "Shell.h"

#include "Core/Logging/Logger.h"
#include "Core/Unicode/Utf.h"
#include "Core/Util/Scope.h"

#include <wil/com.h>
#include <shellapi.h>
#include <shobjidl_core.h>
#include <propkey.h>
#include <propvarutil.h>

using namespace Citrine;

namespace {

	class AddEnvironmentVariablesSite : public winrt::implements<AddEnvironmentVariablesSite, ::IServiceProvider, ::ICreatingProcess> {
	public:

		AddEnvironmentVariablesSite(std::vector<EnvironmentVariable>&& envVariables)

			: envVariables(std::move(envVariables))
		{}

		auto QueryService(::GUID const& guidService, ::IID const& riid, void** ppv) -> ::HRESULT final override {

			if (guidService == SID_ExecuteCreatingProcess) {

				return QueryInterface(riid, ppv);
			}
			else {

				*ppv = nullptr;
				return E_NOTIMPL;
			}
		}

		auto OnCreating(::ICreateProcessInputs* inputs) -> ::HRESULT final override {

			for (auto const& [name, value] : envVariables) {

				auto result = inputs->SetEnvironmentVariableW(ToUtf16(name).c_str(), ToUtf16(value).c_str());
				if (result != S_OK)
					return result;
			}
			return S_OK;
		}

	private:

		std::vector<EnvironmentVariable> envVariables;
	};
}

namespace Citrine::Windows {

	auto Shell::OpenFolderAsync(std::filesystem::path path) -> AsyncShellResult<> {

		co_await winrt::resume_background();

		auto executeInfo = ::SHELLEXECUTEINFOW{
			
			.cbSize = sizeof(::SHELLEXECUTEINFOW),
			.fMask = SEE_MASK_NOASYNC,
			.lpVerb = L"open",
			.lpFile = path.c_str(),
			.nShow = SW_SHOWNORMAL
		};

		if (!::ShellExecuteExW(&executeInfo)) {

			auto lastError = ::GetLastError();
			Logger::Error("Opening folder {} failed: {}", path, lastError);
			co_return std::unexpected{ lastError };
		}

		co_return {};
	}

	auto Shell::ExecuteAsync(std::filesystem::path path, std::string_view arguments, bool runAsAdmin, std::vector<EnvironmentVariable> envVariables) -> AsyncShellResult<wil::unique_process_handle> {

		auto wideArgs = ToUtf16(arguments);

		co_await winrt::resume_background();

		auto addEnvironmentVariablesSite = winrt::make<AddEnvironmentVariablesSite>(std::move(envVariables));
		auto executeInfo = ::SHELLEXECUTEINFOW{

			.cbSize = sizeof(::SHELLEXECUTEINFOW),
			.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_HINST_IS_SITE,
			.lpVerb = runAsAdmin ? L"runas" : L"open",
			.lpFile = path.c_str(),
			.lpParameters = wideArgs.c_str(),
			.nShow = SW_SHOWNORMAL,
			.hInstApp = reinterpret_cast<::HINSTANCE>(addEnvironmentVariablesSite.get())
		};

		if (!::ShellExecuteExW(&executeInfo)) {

			auto lastError = ::GetLastError();
			Logger::Error("Opening {} failed: {}", path, lastError);
			co_return std::unexpected{ lastError };
		}

		co_return wil::unique_process_handle{ executeInfo.hProcess };
	}

	auto Shell::CreateShortcut(std::filesystem::path const& path, std::filesystem::path const& targetPath, std::string_view arguments, std::string_view aumid) -> ShellResult<> {

		auto wideArgs = ToUtf16(arguments);
		auto wideAumid = ToUtf16(aumid);

		auto shellLink = wil::CoCreateInstanceNoThrow<::IShellLinkW>(CLSID_ShellLink);
		if (!shellLink) {

			Logger::Error("Instantiating shell link ({}) failed", path);
			return std::unexpected{ 0x80004002 }; // E_NOINTERFACE
		}

		if (auto result = shellLink->SetPath(targetPath.c_str()); result != S_OK) {

			Logger::Error("Setting target path for shell link ({}) failed: {}", path, result);
			return std::unexpected{ result };
		}

		if (auto result = shellLink->SetArguments(wideArgs.c_str()); result != S_OK) {

			Logger::Error("Setting arguments for shell link ({}) failed: {}", path, result);
			return std::unexpected{ result };
		}

		if (!wideAumid.empty()) {

			auto propertyStore = wil::com_ptr_nothrow<::IPropertyStore>{};
			if (auto result = shellLink->QueryInterface<::IPropertyStore>(std::out_ptr(propertyStore)); result != S_OK) {

				Logger::Error("Accessing property store for shell link ({}) failed: {}", path, result);
				return std::unexpected{ result };
			}

			auto aumidValue = ::PROPVARIANT{};
			::InitPropVariantFromString(wideAumid.c_str(), &aumidValue);

			if (auto result = propertyStore->SetValue(PKEY_AppUserModel_ID, aumidValue); result != S_OK) {

				Logger::Error("Setting aumid for shell link ({}) failed: {}", path, result);
				return std::unexpected{ result };
			}

			if (auto result = propertyStore->Commit(); result != S_OK) {

				Logger::Error("Commiting property store for shell link ({}) failed: {}", path, result);
				return std::unexpected{ result };
			}
		}

		auto persistentShellLink = wil::com_ptr_nothrow<::IPersistFile>{};
		auto result = shellLink->QueryInterface<::IPersistFile>(std::out_ptr(persistentShellLink));
		if (result == S_OK)
			result = persistentShellLink->Save(path.c_str(), true);

		if (result != S_OK) {

			Logger::Error("Commiting shell link ({}) to disk failed: {}", path, result);
			return std::unexpected{ result };
		}
		return {};
	}
}