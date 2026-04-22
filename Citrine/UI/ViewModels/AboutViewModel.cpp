#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif

#include "Citrine.h"
#include "Core/Util/StringLiteral.h"
#include "Locale/Localizer.h"
#include "UI/Mvvm/RelayCommand.h"

#include <winrt/Microsoft.Windows.ApplicationModel.WindowsAppRuntime.h>

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::System;
	using namespace Microsoft::Windows::ApplicationModel;
	using namespace Microsoft::UI::Xaml::Input;
}

namespace winrt::Citrine::implementation
{
	AboutViewModel::AboutViewModel() {

		openHyperlinkCommand = MakeRelayCommand([](winrt::IInspectable const& arg) -> winrt::fire_and_forget {

			try {

				auto hyperlink = arg.try_as<winrt::hstring>();
				if (!hyperlink)
					co_return;

				co_await winrt::Launcher::LaunchUriAsync(winrt::Uri{ *hyperlink });
			}
			catch (winrt::hresult_error const&) {}
		});

		versionInfoString = winrt::format(L"{} {}", Localizer::GetString(L"About_Version"), StringLiteralCast<wchar_t>(CITRINE_PRODUCTVERSION));

		// https://github.com/microsoft/WindowsAppSDK/issues/6387
		// runtimeInfoString = winrt::format(L"Windows App Runtime {}", winrt::WindowsAppRuntime::RuntimeInfo::AsString());
	}

	auto AboutViewModel::OpenHyperlinkCommand() const noexcept -> winrt::ICommand {

		return openHyperlinkCommand;
	}

	auto AboutViewModel::VersionInfoString() const noexcept -> winrt::hstring {

		return versionInfoString;
	}

	auto AboutViewModel::RuntimeInfoString() const noexcept -> winrt::hstring {

		return runtimeInfoString;
	}
}
