#include "pch.h"
#include "MinecraftBedrockGamePackagePicker.h"
#if __has_include("MinecraftBedrockGamePackagePicker.g.cpp")
#include "MinecraftBedrockGamePackagePicker.g.cpp"
#endif

#include "Core/Util/Guid.h"

#include <array>
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
	auto MinecraftBedrockGamePackagePicker::XamlRoot() const noexcept -> winrt::Microsoft::UI::Xaml::XamlRoot {

		return xamlRoot;
	}

	auto MinecraftBedrockGamePackagePicker::XamlRoot(winrt::Microsoft::UI::Xaml::XamlRoot const& value) noexcept -> void {

		xamlRoot = value;
	}

	auto MinecraftBedrockGamePackagePicker::PickGamePackage() -> winrt::hstring {

		auto dialog = wil::CoCreateInstance<::IFileOpenDialog>(CLSID_FileOpenDialog);
		auto opts = ::FILEOPENDIALOGOPTIONS{};

		static constexpr auto guid = "6e37fe78-5ea2-45b6-acdb-50488e289c39"_Guid;
		dialog->SetClientGuid(guid);

		dialog->GetOptions(&opts);
		opts |= FOS_FORCEFILESYSTEM;
		opts |= FOS_FILEMUSTEXIST;
		dialog->SetOptions(opts);

		static constexpr auto fileTypes = std::to_array<::COMDLG_FILTERSPEC>({

			{ L"msixvc | appx", L"*.msixvc;*.appx" },
			{ L"msixvc", L"*.msixvc" },
			{ L"appx", L"*.appx" },
		});
		dialog->SetFileTypes(fileTypes.size(), fileTypes.data());

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
