#include "pch.h"
#include "MinecraftBedrockInstallLocationPicker.h"
#if __has_include("MinecraftBedrockInstallLocationPicker.g.cpp")
#include "MinecraftBedrockInstallLocationPicker.g.cpp"
#endif

#include "Core/Util/Guid.h"

#include <memory>

#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Interop.h>

#include <wil/com.h>

#include <shobjidl_core.h>

using namespace Citrine;

namespace winrt {

    using namespace Microsoft::UI;
    using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
    auto MinecraftBedrockInstallLocationPicker::XamlRoot() const noexcept -> winrt::XamlRoot {

        return xamlRoot;
    }

    auto MinecraftBedrockInstallLocationPicker::XamlRoot(winrt::XamlRoot const& value) noexcept -> void {

        xamlRoot = value;
    }

    auto MinecraftBedrockInstallLocationPicker::DefaultInstallLocation() const noexcept -> winrt::hstring {

        return defaultInstallLocation;
    }

    auto MinecraftBedrockInstallLocationPicker::DefaultInstallLocation(winrt::hstring const& value) noexcept -> void {

        defaultInstallLocation = value;
    }

    auto MinecraftBedrockInstallLocationPicker::PickInstallLocation() -> winrt::hstring {

        auto dialog = wil::CoCreateInstance<::IFileOpenDialog>(CLSID_FileOpenDialog);
        auto opts = ::FILEOPENDIALOGOPTIONS{};

        static constexpr auto guid = "c5c6d49c-5dde-45c7-a0f1-f8203c02241b"_Guid;
        dialog->SetClientGuid(guid);

        dialog->GetOptions(&opts);
        opts |= FOS_PICKFOLDERS;
        opts |= FOS_PATHMUSTEXIST;
        dialog->SetOptions(opts);

        auto defaultFolder = wil::com_ptr<::IShellItem>();
        ::SHCreateItemFromParsingName(defaultInstallLocation.c_str(), nullptr, IID_IShellItem, std::out_ptr(defaultFolder));
        dialog->SetDefaultFolder(defaultFolder.get());

        auto currentWindow = winrt::GetWindowFromWindowId(xamlRoot.ContentIslandEnvironment().AppWindowId());
        if (dialog->Show(currentWindow) != S_OK)
            return {};

        auto result = wil::com_ptr<::IShellItem>{};
        dialog->GetResult(std::out_ptr(result));

        auto path = wil::unique_cotaskmem_string{};
        result->GetDisplayName(SIGDN_FILESYSPATH, std::out_ptr(path));
        return path.get();
    }
}
