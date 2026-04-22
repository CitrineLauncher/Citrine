#pragma once

#include "ApplicationSettings.h"

#include <filesystem>

namespace Citrine {

	class ApplicationData {
	public:

		static auto Initialize() -> void;

		static auto LocalDirectory() noexcept -> std::filesystem::path const&;
		static auto LocalLogDirectory() noexcept -> std::filesystem::path const&;
		static auto LocalCacheDirectory() noexcept -> std::filesystem::path const&;
		static auto LocalSettings() noexcept -> LocalApplicationSettings&;

	private:

		static std::filesystem::path localDirectory;
		static std::filesystem::path localLogDirectory;
		static std::filesystem::path localCacheDirectory;
		static LocalApplicationSettings localSettings;
	};
}

