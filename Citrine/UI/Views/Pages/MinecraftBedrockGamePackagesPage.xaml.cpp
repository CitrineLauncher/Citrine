#include "pch.h"
#include "MinecraftBedrockGamePackagesPage.xaml.h"
#if __has_include("MinecraftBedrockGamePackagesPage.g.cpp")
#include "MinecraftBedrockGamePackagesPage.g.cpp"
#endif

#include "Models/ParameterPair.h"
#include "UI/Views/Dialogs/MinecraftBedrockInstallDialog.xaml.h"
#include "UI/Views/Dialogs/MinecraftBedrockManageDialog.xaml.h"
#include "UI/Views/Dialogs/MinecraftBedrockGamePackagePicker.h"
#include "UI/Views/Dialogs/MinecraftBedrockImportDialog.xaml.h"
#include "UI/Views/Dialogs/MinecraftBedrockRenameDialog.xaml.h"

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
	MinecraftBedrockGamePackagesPage::MinecraftBedrockGamePackagesPage(winrt::hstring packageSourceId) 

		: viewModel(winrt::make_self<implementation::MinecraftBedrockGamePackagesViewModel>(std::move(packageSourceId)))
	{
		Loaded([this](auto const&...) {

			OnLoaded();
		});
	}

	auto MinecraftBedrockGamePackagesPage::InvokeGamePackageActionCommand_ExecuteRequested(winrt::XamlUICommand const&, winrt::ExecuteRequestedEventArgs args) -> winrt::fire_and_forget try {

		auto pair = args.Parameter().try_as<implementation::ParameterPair>();
		if (!pair)
			co_return;

		auto action = pair->first.try_as<winrt::hstring>();
		if (!action)
			co_return;

		auto gamePackage = pair->second.try_as<Citrine::MinecraftBedrockGamePackageItem>();
		auto launchArgs = pair->second.try_as<Citrine::MinecraftBedrockGameLaunchArgs>();

		if (action == L"Install") {

			if (!gamePackage)
				co_return;

			auto dialog = Citrine::MinecraftBedrockInstallDialog{ *viewModel, gamePackage };
			dialog.XamlRoot(XamlRoot());
			co_await dialog.ShowAsync();
		}
		else if (action == L"Launch") {

			if (!launchArgs)
				co_return;

			viewModel->LaunchGamePackage(launchArgs);
		}
		else if (action == L"PauseOperation") {

			if (!gamePackage)
				co_return;

			viewModel->PauseGamePackageOperation(gamePackage);
		}
		else if (action == L"ResumeOperation") {

			if (!gamePackage)
				co_return;

			viewModel->ResumeGamePackageOperation(gamePackage);
		}
		else if (action == L"CancelOperation") {

			if (!gamePackage)
				co_return;

			viewModel->CancelGamePackageOperation(gamePackage);
		}
		else if (action == L"Rename") {

			if (!gamePackage)
				co_return;

			auto dialog = Citrine::MinecraftBedrockRenameDialog{ *viewModel, gamePackage };
			dialog.XamlRoot(XamlRoot());
			co_await dialog.ShowAsync();
		}
		else if (action == L"OpenGameDirectory") {

			if (!gamePackage)
				co_return;

			viewModel->OpenGameDirectory(gamePackage);
		}
		else if (action == L"OpenGameDataDirectory") {

			if (!gamePackage)
				co_return;

			viewModel->OpenGameDataDirectory(gamePackage);
		}
		else if (action == L"Manage") {

			if (!gamePackage)
				co_return;

			auto dialog = Citrine::MinecraftBedrockManageDialog{ *viewModel, gamePackage };
			dialog.XamlRoot(XamlRoot());
			co_await dialog.ShowAsync();
		}
	}
	catch (winrt::hresult_error const&) {} 

	auto MinecraftBedrockGamePackagesPage::ImportButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> winrt::fire_and_forget {

		auto picker = winrt::make_self<implementation::MinecraftBedrockGamePackagePicker>();
		picker->XamlRoot(XamlRoot());

		auto gamePackageLocation = picker->PickGamePackage();
		if (gamePackageLocation.empty())
			co_return;

		auto dialog = Citrine::MinecraftBedrockImportDialog{ *viewModel, gamePackageLocation };
		dialog.XamlRoot(XamlRoot());
		co_await dialog.ShowAsync();
	}

	auto MinecraftBedrockGamePackagesPage::ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockGamePackagesPage::OnLoaded() -> void {

		if (contentArea)
			return;

		auto grid = Content().as<winrt::Grid>();
		contentArea = grid;

		for (auto const& child : grid.Children()) {

			auto frameworkElement = child.as<winrt::FrameworkElement>();
			auto margin = frameworkElement.Margin();

			baseContentHeight += margin.Top;
			baseContentHeight += margin.Bottom;

			if (!child.try_as<winrt::ScrollViewer>())
				baseContentHeight += frameworkElement.ActualHeight();
		}

		SizeChanged([this](auto const&, auto const& args) {

			auto newSize = args.NewSize();
			OnSizeChanged(newSize.Width, newSize.Height);
		});
		gamePackagesChangedRevoker = viewModel->GamePackages().CollectionChanged(winrt::auto_revoke, [this](auto const&...) {
			
			auto actualSize = ActualSize();
			OnSizeChanged(actualSize.x, actualSize.y);
		});

		auto actualSize = ActualSize();
		OnSizeChanged(actualSize.x, actualSize.y);
	}

	auto MinecraftBedrockGamePackagesPage::OnSizeChanged(double newWidth, double newHeight) -> void {

		auto contentHeight = baseContentHeight + (static_cast<double>(viewModel->GamePackages().Size()) * (110 + 3));
		contentArea.Margin(winrt::Thickness{ 0, std::max((newHeight - contentHeight) / 2, 0.0), 0, 0 });
	}
}
