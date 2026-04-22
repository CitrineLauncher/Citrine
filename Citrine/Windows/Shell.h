#pragma once

#include "Core/Coroutine/Task.h"

#include <string>
#include <expected>
#include <filesystem>

namespace Citrine::Windows {

	template<typename T = void>
	using ShellResult = std::expected<T, std::uint32_t>;

	template<typename T = void>
	using AsyncShellResult = Task<ShellResult<T>>;

	class Shell {
	public:

		static auto OpenFolderAsync(std::filesystem::path path) -> AsyncShellResult<>;
		static auto ExecuteAsync(std::filesystem::path path, std::string_view arguments) -> AsyncShellResult<std::uint32_t>;
		static auto CreateShortcut(std::filesystem::path const& path, std::filesystem::path const& targetPath, std::string_view arguments, std::string_view aumid) -> ShellResult<>;
	};
}
