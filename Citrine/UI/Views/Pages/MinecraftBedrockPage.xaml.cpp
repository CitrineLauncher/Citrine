#include "pch.h"
#include "MinecraftBedrockPage.xaml.h"
#if __has_include("MinecraftBedrockPage.g.cpp")
#include "MinecraftBedrockPage.g.cpp"
#endif

#include "UI/Navigation/NavigationContext.h"

namespace winrt {

	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	MinecraftBedrockPage::MinecraftBedrockPage() {

		Loaded([this](auto const&...) {

			OnLoaded();
		});
	}

	auto MinecraftBedrockPage::InitializeComponent() -> void {

		MinecraftBedrockPageT::InitializeComponent();

		auto frame = Frame();
		navMap.try_emplace(L"Releases", frame, winrt::xaml_typename<Citrine::MinecraftBedrockReleasesPage>());
		navMap.try_emplace(L"Previews", frame, winrt::xaml_typename<Citrine::MinecraftBedrockPreviewsPage>());
		navMap.try_emplace(L"Imports", frame, winrt::xaml_typename<Citrine::MinecraftBedrockImportsPage>());

		auto navView = NavView();
		navView.SelectedItem(navView.MenuItems().GetAt(0));
	}

	auto MinecraftBedrockPage::ViewModel() const noexcept -> Citrine::MinecraftBedrockViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockPage::NavView_SelectionChanged(winrt::NavigationView const&, winrt::NavigationViewSelectionChangedEventArgs const& args) -> void {

		auto navItem = args.SelectedItem().as<winrt::NavigationViewItem>();
		auto tag = winrt::unbox_value<winrt::hstring>(navItem.Tag());

		auto it = navMap.find(tag);
		if (it != navMap.end()) {

			auto context = winrt::get_self<implementation::NavigationContext>(it->second);
			context->MakeActive(args.RecommendedNavigationTransitionInfo());
			activePage = Frame().Content().try_as<winrt::Page>();

			auto actualSize = ActualSize();
			OnSizeChanged(actualSize.x, actualSize.y);

			auto visualState = std::wstring_view{ L"ViewModeToggleVisible" };
			if (activePage.try_as<Citrine::MinecraftBedrockImportsPage>())
				visualState = L"ViewModeToggleCollapsed";

			winrt::VisualStateManager::GoToState(*this, visualState, true);
		}
	}

	auto MinecraftBedrockPage::OnLoaded() -> void {

		{
			auto viewModeToggle = ViewModeToggle();
			auto actualWidth = viewModeToggle.ActualWidth();
			auto margin = viewModeToggle.Margin();

			viewModeToggleWidth = margin.Left + actualWidth + margin.Right;
		}

		SizeChanged([this](auto const&, auto const& args) {

			auto newSize = args.NewSize();
			OnSizeChanged(newSize.Width, newSize.Height);
		});

		auto actualSize = ActualSize();
		OnSizeChanged(actualSize.x, actualSize.y);
	}

	auto MinecraftBedrockPage::OnSizeChanged(double newWidth, double newHeight) -> void {

		if (!activePage || activePage.try_as<Citrine::MinecraftBedrockImportsPage>())
			return;

		auto sufficientSpace = (newWidth - 720) / 2 >= viewModeToggleWidth;
		activePage.Margin({ 0.0, 0.0, 0.0, sufficientSpace ? 0.0 : 32.0 });
	}
}
