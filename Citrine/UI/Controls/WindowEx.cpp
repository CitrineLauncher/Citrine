#include "pch.h"
#include "WindowEx.h"
#if __has_include("WindowEx.g.cpp")
#include "WindowEx.g.cpp"
#endif

#include "ApplicationData.h"

#include <commctrl.h>
#pragma comment (lib, "comctl32.lib")

#include <microsoft.ui.xaml.window.h>

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::UI;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Composition::SystemBackdrops;
}

namespace winrt::Citrine::implementation
{
	WindowEx::WindowEx() {

		if (auto windowNative = this->try_as<::IWindowNative>()) {

			windowNative->get_WindowHandle(&handle);
			::SetWindowSubclass(handle, &SubclassProc, 0, reinterpret_cast<::DWORD_PTR>(this));
		}

		auto& appSettings = ApplicationData::LocalSettings();

		requestedThemeChangedRevoker = appSettings.ThemeChanged([this](AppTheme value) {

			contentPresenter.RequestedTheme(static_cast<winrt::ElementTheme>(value));
		});
		contentPresenter.RequestedTheme(static_cast<winrt::ElementTheme>(appSettings.Theme()));

		requestedBackdropChangedRevoker = appSettings.BackdropChanged([this](AppBackdrop value) {

			backdrop.Kind(static_cast<winrt::MicaKind>(value));
		});
		backdrop.Kind(static_cast<winrt::MicaKind>(appSettings.Backdrop()));

		auto appTitleBar = AppWindow().TitleBar();
		appTitleBar.ExtendsContentIntoTitleBar(true);
		appTitleBar.ButtonBackgroundColor(winrt::Color{ 0x00, 0xFF, 0xFF, 0xFF });
		appTitleBar.ButtonInactiveBackgroundColor(winrt::Color{ 0x00, 0xFF, 0xFF, 0xFF });

		actualThemeChangedRevoker = contentPresenter.ActualThemeChanged(winrt::auto_revoke, [this](auto const&...) {

			OnActualThemeChanged(contentPresenter.ActualTheme());
		});
		OnActualThemeChanged(contentPresenter.ActualTheme());

		Content(contentPresenter);
		SystemBackdrop(backdrop);
	}

	auto WindowEx::WindowContent() const -> winrt::IInspectable {

		return contentPresenter.Content();
	}

	auto WindowEx::WindowContent(winrt::IInspectable const& value) -> void {

		contentPresenter.Content(value);
	}

	auto WindowEx::NativeHandle() const -> ::HWND {

		return handle;
	}

	auto WindowEx::OnWindowMessage(::HWND, ::UINT, ::WPARAM, ::LPARAM) -> void {


	}

	WindowEx::~WindowEx() noexcept {

		::RemoveWindowSubclass(handle, &SubclassProc, 0);
	}

	auto WindowEx::OnActualThemeChanged(winrt::ElementTheme theme) -> void {

		UpdateCaptionButtonColors(theme);
		UpdateContextMenuTheme(theme);
	}

	auto WindowEx::UpdateCaptionButtonColors(winrt::ElementTheme theme) -> void {

		auto appTitleBar = AppWindow().TitleBar();

		if (theme == winrt::ElementTheme::Dark) {

			appTitleBar.ButtonForegroundColor(winrt::Color{ 0xFF, 0xFF, 0xFF, 0xFF });
			appTitleBar.ButtonHoverBackgroundColor(winrt::Color{ 0x15, 0xFF, 0xFF, 0xFF });
			appTitleBar.ButtonHoverForegroundColor(winrt::Color{ 0xFF, 0xFF, 0xFF, 0xFF });
			appTitleBar.ButtonPressedBackgroundColor(winrt::Color{ 0x0F, 0xFF, 0xFF, 0xFF });
			appTitleBar.ButtonPressedForegroundColor(winrt::Color{ 0xFF, 0xD3, 0xD3, 0xD3 });
		}
		else {

			appTitleBar.ButtonForegroundColor(winrt::Color{ 0xFF, 0x00, 0x00, 0x00 });
			appTitleBar.ButtonHoverBackgroundColor(winrt::Color{ 0x0A, 0x00, 0x00, 0x00 });
			appTitleBar.ButtonHoverForegroundColor(winrt::Color{ 0xFF, 0x00, 0x00, 0x00 });
			appTitleBar.ButtonPressedBackgroundColor(winrt::Color{ 0x15, 0x00, 0x00, 0x00 });
			appTitleBar.ButtonPressedForegroundColor(winrt::Color{ 0xFF, 0x69, 0x69, 0x69 });
		}
	}

	auto WindowEx::UpdateContextMenuTheme(winrt::Microsoft::UI::Xaml::ElementTheme theme) -> void {

		enum struct PreferredAppMode
		{
			Default,
			AllowDark,
			ForceDark,
			ForceLight
		};

		auto uxtheme = ::LoadLibraryW(L"uxtheme.dll");
		if (!uxtheme)
			return;

		auto setPreferredAppMode = reinterpret_cast<auto (WINAPI*)(PreferredAppMode) -> PreferredAppMode>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
		auto flushMenuThemes = reinterpret_cast<auto (WINAPI*)() -> void>(::GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));

		setPreferredAppMode(theme == winrt::ElementTheme::Dark ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
		flushMenuThemes();

		::FreeLibrary(uxtheme);
	}

	auto CALLBACK WindowEx::SubclassProc(::HWND hWnd, ::UINT uMsg, ::WPARAM wParam, ::LPARAM lParam, ::UINT_PTR, ::DWORD_PTR dwRefData) -> ::LRESULT {

		if (uMsg == WM_GETMINMAXINFO) {

			auto scaleFactor = static_cast<double>(::GetDpiForWindow(hWnd)) / USER_DEFAULT_SCREEN_DPI;
			auto info = reinterpret_cast<::LPMINMAXINFO>(lParam);
			info->ptMinTrackSize.x = 680 * scaleFactor;
			info->ptMinTrackSize.y = 420 * scaleFactor;
		}

		reinterpret_cast<WindowEx*>(dwRefData)->OnWindowMessage(hWnd, uMsg, wParam, lParam);
		return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
}
