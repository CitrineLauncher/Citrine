#include "pch.h"
#include "ApplicationData.h"

#include <memory>

#include <shlobj.h>
#pragma comment (lib, "shell32.lib")

#include <wil/resource.h>

namespace Citrine {

	auto ApplicationData::Initialize() -> void {

		{
			auto path = wil::unique_cotaskmem_string{};
			if (::SHGetKnownFolderPath(::FOLDERID_LocalAppData, 0, nullptr, std::out_ptr(path)) == S_OK)
				localDirectory = path.get();
		}

		localDirectory /= L"CitrineLauncher";
		std::filesystem::create_directory(localDirectory);

		localLogDirectory = localDirectory / L"Logs";
		std::filesystem::create_directory(localLogDirectory);

		localCacheDirectory = localDirectory / L"Cache";
		std::filesystem::create_directory(localCacheDirectory);

		localSettings.Initialize(localDirectory / L"Settings.json");
	}

	auto ApplicationData::LocalDirectory() noexcept -> std::filesystem::path const& {

		return localDirectory;
	}

	auto ApplicationData::LocalLogDirectory() noexcept -> std::filesystem::path const& {

		return localLogDirectory;
	}

	auto ApplicationData::LocalCacheDirectory() noexcept -> std::filesystem::path const& {

		return localCacheDirectory;
	}

	auto ApplicationData::LocalSettings() noexcept -> LocalApplicationSettings& {

		return localSettings;
	}

	std::filesystem::path ApplicationData::localDirectory;
	std::filesystem::path ApplicationData::localLogDirectory;
	std::filesystem::path ApplicationData::localCacheDirectory;
	LocalApplicationSettings ApplicationData::localSettings;
}