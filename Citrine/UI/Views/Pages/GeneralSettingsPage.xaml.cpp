#include "pch.h"
#include "GeneralSettingsPage.xaml.h"
#if __has_include("GeneralSettingsPage.g.cpp")
#include "GeneralSettingsPage.g.cpp"
#endif

#include "Core/Logging/Logger.h"

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
	using namespace Microsoft::UI::Xaml::Input;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	GeneralSettingsPage::GeneralSettingsPage() {

		Loaded([this](auto const&...) {

			OnLoaded();
		});
	}

	auto GeneralSettingsPage::ViewModel() const noexcept -> Citrine::GeneralSettingsViewModel {

		return *viewModel;
	}

	auto GeneralSettingsPage::OnLoaded() -> void {

		if (contentArea)
			return;

		auto scrollView = Content().as<winrt::ScrollViewer>();
		auto stackPanel = scrollView.Content().as<winrt::StackPanel>();
		contentArea = stackPanel;

		for (auto const& child : stackPanel.Children()) {

			auto frameworkElement = child.as<winrt::FrameworkElement>();
			auto margin = frameworkElement.Margin();

			baseContentHeight += margin.Top;
			baseContentHeight += margin.Bottom;

			if (!child.try_as<winrt::InfoBar>())
				baseContentHeight += frameworkElement.ActualHeight();
		}

		SizeChanged([this](auto const&, auto const& args) {

			auto newSize = args.NewSize();
			OnSizeChanged(newSize.Width, newSize.Height);
		});

		auto actualSize = ActualSize();
		OnSizeChanged(actualSize.x, actualSize.y);
	}

	auto GeneralSettingsPage::OnSizeChanged(double newWidth, double newHeight) -> void {

		contentArea.Margin(winrt::Thickness{ 0, std::max((newHeight - baseContentHeight) / 2, 0.0), 0, 0 });
	}
}
