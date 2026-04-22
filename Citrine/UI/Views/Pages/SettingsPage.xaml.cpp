#include "pch.h"
#include "SettingsPage.xaml.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

namespace winrt {

	using namespace Microsoft::UI::Xaml::Controls;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	auto SettingsPage::InitializeComponent() -> void {

		SettingsPageT::InitializeComponent();

		auto frame = Frame();
		navMap.try_emplace(L"General", frame, winrt::xaml_typename<Citrine::GeneralSettingsPage>());
		navMap.try_emplace(L"About", frame, winrt::xaml_typename<Citrine::AboutPage>());

		auto navView = NavView();
		navView.SelectedItem(navView.MenuItems().GetAt(0));
	}

	auto SettingsPage::NavView_SelectionChanged(winrt::NavigationView const&, winrt::NavigationViewSelectionChangedEventArgs const& args) -> void {

		auto navItem = args.SelectedItem().as<winrt::NavigationViewItem>();
		auto tag = winrt::unbox_value<winrt::hstring>(navItem.Tag());

		auto it = navMap.find(tag);
		if (it != navMap.end()) {

			auto& context = it->second;
			context.MakeActive(args.RecommendedNavigationTransitionInfo());
		}
	}
}
