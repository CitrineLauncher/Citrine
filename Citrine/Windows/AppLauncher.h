#pragma once

#include "Core/Coroutine/Task.h"

#include <string>
#include <expected>
#include <filesystem>

namespace Citrine::Windows {

	using AppLaunchResult = std::expected<std::uint32_t, std::int32_t>;
	using AsyncAppLaunchResult = Task<AppLaunchResult>;

	class AppLauncher {
	public:

		static auto LaunchAsync(std::string_view aumid, std::string_view arguments = {}) -> AsyncAppLaunchResult;
		static auto LaunchForFileAsync(std::string_view aumid, std::filesystem::path filePath, std::string_view verb = {}) -> AsyncAppLaunchResult;
	};
}