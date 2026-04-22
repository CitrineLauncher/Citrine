#include "pch.h"
#include "ContentDialogEx.h"
#if __has_include("ContentDialogEx.g.cpp")
#include "ContentDialogEx.g.cpp"
#endif

#include "App.Xaml.h"
#include "ApplicationData.h"

using namespace Citrine;

namespace winrt {

	using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
	ContentDialogEx::ContentDialogEx() {

		auto appResources = App::Current().Resources();
		Style(appResources.Lookup(winrt::box_value(L"DefaultContentDialogStyle")).as<winrt::Style>());

		auto& appSettings = ApplicationData::LocalSettings();

		requestedThemeChangedRevoker = appSettings.ThemeChanged([this](AppTheme value) {

			RequestedTheme(static_cast<winrt::ElementTheme>(value));
		});
		RequestedTheme(static_cast<winrt::ElementTheme>(appSettings.Theme()));
	}
}
