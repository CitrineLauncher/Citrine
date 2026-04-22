#pragma once

#include <string>
#include <filesystem>

namespace Citrine::Windows {

	struct User {

		std::string Sid;
		std::string Name;
		std::filesystem::path LocalAppDataDirectory;
		std::filesystem::path RoamingAppDataDirectory;
	};

	auto GetCurrentUser() -> User const&;
}
