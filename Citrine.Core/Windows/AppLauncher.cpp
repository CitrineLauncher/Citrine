#include "pch.h"
#include "AppLauncher.h"

#include "Core/Unicode/Utf.h"
#include "Core/Logging/Logger.h"

#include <algorithm>
#include <memory>

#include <wil/com.h>
#include <shobjidl_core.h>
#include <objbase.h>
#pragma comment(lib, "ole32.lib")

namespace {

	auto GetActivationManager() -> ::IApplicationActivationManager* {

		static auto activationManager = wil::CoCreateInstanceNoThrow<::IApplicationActivationManager>(::CLSID_ApplicationActivationManager, ::CLSCTX_INPROC_SERVER);
		return activationManager.get();
	}
}

namespace Citrine::Windows {

	auto AppLauncher::LaunchAsync(std::string_view aumid, std::string_view arguments) -> AsyncAppLaunchResult {

		auto wideAumid = ToUtf16(aumid);
		auto wideArgs = ToUtf16(arguments);

		co_await winrt::resume_background();

		auto activationManager = GetActivationManager();
		if (!activationManager) {

			Logger::Error("Instantiating ApplicationActivationManager failed");
			co_return std::unexpected{ 0x80004002 }; // E_NOINTERFACE
		}

		auto input = ::INPUT{ .type = INPUT_KEYBOARD, .ki = {} };
		::SendInput(1, &input, sizeof(input));
		::CoAllowSetForegroundWindow(activationManager, nullptr);

		auto processId = ::DWORD{};;
		if (auto result = activationManager->ActivateApplication(wideAumid.c_str(), wideArgs.c_str(), ::AO_NONE, &processId); result != S_OK) {

			Logger::Error(L"Activating {} failed: {}", wideAumid, result);
			co_return std::unexpected{ result };
		}
		co_return processId;
	}

	auto AppLauncher::LaunchForFileAsync(std::string_view aumid, std::filesystem::path filePath, std::string_view verb) -> AsyncAppLaunchResult {

		auto wideAumid = ToUtf16(aumid);
		auto wideVerb = ToUtf16(verb);

		co_await winrt::resume_background();

		auto activationManager = GetActivationManager();
		if (!activationManager) {

			Logger::Error("Instantiating ApplicationActivationManager failed");
			co_return std::unexpected{ 0x80004002 }; // E_NOINTERFACE
		}

		auto shellItem = wil::com_ptr_nothrow<::IShellItem>{ nullptr };
		if (auto result = ::SHCreateItemFromParsingName(filePath.c_str(), nullptr, __uuidof(::IShellItem), std::out_ptr(shellItem)); result != S_OK) {

			Logger::Info("Creating shell item for file ({}) failed: {}", filePath, result);
			co_return std::unexpected{ result };
		}

		auto shellItemArray = wil::com_ptr_nothrow<::IShellItemArray>{};
		if (auto result = ::SHCreateShellItemArrayFromShellItem(shellItem.get(), __uuidof(::IShellItemArray), std::out_ptr(shellItemArray)); result != S_OK) {

			Logger::Info("Creating shell item array for file ({}) failed: {}", filePath, result);
			co_return std::unexpected{ result };
		}

		auto input = ::INPUT{ .type = INPUT_KEYBOARD, .ki = {} };
		::SendInput(1, &input, sizeof(input));
		::CoAllowSetForegroundWindow(activationManager, nullptr);

		auto processId = ::DWORD{};
		if (auto result = activationManager->ActivateForFile(wideAumid.c_str(), shellItemArray.get(), wideVerb.c_str(), &processId); result != S_OK) {

			Logger::Error(L"Activating {} failed: {}", wideAumid, result);
			co_return std::unexpected{ result };
		}
		co_return processId;
	}

	auto AppLauncher::EnableDebugging(std::string_view packageFullName) -> bool {

		auto packageDebugSettings = wil::CoCreateInstanceNoThrow<IPackageDebugSettings>(CLSID_PackageDebugSettings, CLSCTX_INPROC_SERVER);
		if (!packageDebugSettings)
			return false;

		return packageDebugSettings->EnableDebugging(ToUtf16(packageFullName).data(), nullptr, nullptr) == S_OK;
	}

	auto AppLauncher::DisableDebugging(std::string_view packageFullName) -> bool {

		auto packageDebugSettings = wil::CoCreateInstanceNoThrow<IPackageDebugSettings>(CLSID_PackageDebugSettings, CLSCTX_INPROC_SERVER);
		if (!packageDebugSettings)
			return false;

		return packageDebugSettings->DisableDebugging(ToUtf16(packageFullName).data()) == S_OK;
	}
}