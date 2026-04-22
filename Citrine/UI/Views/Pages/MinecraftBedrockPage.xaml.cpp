#include "pch.h"
#include "MinecraftBedrockPage.xaml.h"
#if __has_include("MinecraftBedrockPage.g.cpp")
#include "MinecraftBedrockPage.g.cpp"
#endif

namespace winrt {

	using namespace Microsoft::UI::Xaml::Controls;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	auto MinecraftBedrockPage::InitializeComponent() -> void {

		MinecraftBedrockPageT::InitializeComponent();

		auto frame = Frame();
		navMap.try_emplace(L"Releases", frame, winrt::xaml_typename<Citrine::MinecraftBedrockReleasesPage>());
		navMap.try_emplace(L"Previews", frame, winrt::xaml_typename<Citrine::MinecraftBedrockPreviewsPage>());
		navMap.try_emplace(L"Imports", frame, winrt::xaml_typename<Citrine::MinecraftBedrockImportsPage>());

		auto navView = NavView();
		navView.SelectedItem(navView.MenuItems().GetAt(0));
	}

	auto MinecraftBedrockPage::NavView_SelectionChanged(winrt::NavigationView const&, winrt::NavigationViewSelectionChangedEventArgs const& args) -> void {

		auto navItem = args.SelectedItem().as<winrt::NavigationViewItem>();
		auto tag = winrt::unbox_value<winrt::hstring>(navItem.Tag());

		auto it = navMap.find(tag);
		if (it != navMap.end()) {

			auto& context = it->second;
			context.MakeActive(args.RecommendedNavigationTransitionInfo());
		}
	}
}
