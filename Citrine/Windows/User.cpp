#include "pch.h"
#include "User.h"

#include "Core/Unicode/Utf.h"

#include <memory>

#include <wil/resource.h>
#include <wil/token_helpers.h>

#include <windows.h>
#pragma comment(lib, "Advapi32.lib")
#include <sddl.h>
#include <shlobj.h>
#pragma comment (lib, "shell32.lib")

namespace Citrine::Windows {

	auto GetCurrentUser() -> User const& {

		static auto defaultUser = [] static {

			auto user = User{};
			{
				auto sid = wil::unique_hlocal_string{};
				if (::ConvertSidToStringSidW(wil::get_token_information<::TOKEN_USER>()->User.Sid, std::out_ptr(sid)))
					user.Sid = ToUtf8(sid.get());
			}
			{
				auto name = std::array<wchar_t, UNLEN + 1>{};
				if (auto size = static_cast<::DWORD>(name.size()); ::GetUserNameW(name.data(), &size))
					user.Name = ToUtf8({ name.data(), size - 1 });
			}
			{
				auto localAppData = wil::unique_cotaskmem_string{};
				if (::SHGetKnownFolderPath(::FOLDERID_LocalAppData, 0, nullptr, std::out_ptr(localAppData)) == S_OK)
					user.LocalAppDataDirectory = localAppData.get();
			}
			{
				auto roamingAppData = wil::unique_cotaskmem_string{};
				if (::SHGetKnownFolderPath(::FOLDERID_RoamingAppData, 0, nullptr, std::out_ptr(roamingAppData)) == S_OK)
					user.RoamingAppDataDirectory = roamingAppData.get();
			}
			return user;
		}();
		return defaultUser;
	}
}