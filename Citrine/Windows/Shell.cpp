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

	auto Shell::ExecuteAsync(std::filesystem::path path, std::string_view arguments) -> AsyncShellResult<std::uint32_t> {

		auto wideArgs = ToUtf16(arguments);

		co_await winrt::resume_background();

		auto executeInfo = ::SHELLEXECUTEINFOW{

			.cbSize = sizeof(::SHELLEXECUTEINFOW),
			.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC,
			.lpVerb = L"open",
			.lpFile = path.c_str(),
			.lpParameters = wideArgs.c_str(),
			.nShow = SW_SHOWNORMAL
		};

		if (!::ShellExecuteExW(&executeInfo)) {

			auto lastError = ::GetLastError();
			Logger::Error("Opening {} failed: {}", path, lastError);
			co_return std::unexpected{ lastError };
		}

		auto closeProcHandle = ScopeExit{ [&] { ::CloseHandle(executeInfo.hProcess); } };
		co_return ::GetProcessId(executeInfo.hProcess);
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