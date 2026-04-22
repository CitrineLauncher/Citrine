#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "App.xaml.h"
#include "ApplicationData.h"

#include "Core/Util/Scope.h"
#include "Models/MinecraftBedrockGameInfo.h"

#include <ranges>
#include "Core/Logging/Logger.h"

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Graphics;
	using namespace Windows::ApplicationModel::DataTransfer;
	using namespace Windows::Storage;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
	using namespace Microsoft::UI::Xaml::Media::Imaging;
}

using namespace ::Citrine;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	auto MainWindow::InitializeComponent() -> void {

		MainWindowT::InitializeComponent();

		redirectEventId = RegisterWindowMessageW(App::RedirectEventName);

		titleBarSizeChangedRevoker = TitleBar().SizeChanged(winrt::auto_revoke, [this](auto const&...) {

			UpdateTitleBarDragArea();
		});

		frame.Margin(winrt::Thickness{ 0, 38, 0, 0 });
		frame.CornerRadius(winrt::CornerRadius{ 1, 1, 1, 1 }); // nested frames clip out if corner radius is 0
		frame.Padding(winrt::Thickness{ 20, 0, 20, 20 });

		auto navView = NavView();
		navView.Content(frame);

		/*
		auto& settings = ApplicationData::LocalSettings();
		if (settings.LandingPage() == "Bedrock")
			navView.SelectedItem(BedrockNavItem());
		else
			navView.SelectedItem(JavaNavItem());
		*/
		navView.SelectedItem(BedrockNavItem());
	}

	auto MainWindow::ViewModel() const noexcept -> Citrine::MainWindowViewModel {

		return *viewModel;
	}

	auto MainWindow::ContentRoot_DragEnter(winrt::IInspectable const&, winrt::DragEventArgs args) -> winrt::fire_and_forget {

		auto dataView = args.DataView();
		if (!dataView.Contains(winrt::StandardDataFormats::StorageItems()))
			co_return;

		auto defferal = args.GetDeferral();
		auto completeDefferal = ScopeExit{ [&] { defferal.Complete(); } };
		args.AcceptedOperation(winrt::DataPackageOperation::None);

		auto strongSelf = get_strong();
		auto storageItems = co_await dataView.GetStorageItemsAsync();

		if (storageItems.Size() == 1) {

			auto storageFile = storageItems.GetAt(0).try_as<winrt::StorageFile>();
			if (!storageFile)
				co_return;

			auto fileType = storageFile.FileType();
			auto associatedFileTypes = implementation::MinecraftBedrockGameInfo::AssociatedFileTypes();

			for (auto associatedFileType : associatedFileTypes) {

				constexpr auto toLower = [](wchar_t ch) static { return Ascii::ToLower(ch); };
				if (std::ranges::equal(associatedFileType, fileType, {}, toLower, toLower)) {

					auto appResources = App::Current().Resources();
					auto iconKey = winrt::box_value(L"MinecraftBedrockPackIcon");
					auto icon = appResources.Lookup(iconKey).as<winrt::BitmapImage>();

					auto oldPixelHeight = icon.DecodePixelHeight();
					auto oldPixelWidth = icon.DecodePixelWidth();

					auto scaleFactor = static_cast<double>(::GetDpiForWindow(NativeHandle())) / USER_DEFAULT_SCREEN_DPI;
					auto pixelHeight = static_cast<std::int32_t>(64 * scaleFactor);
					auto pixelWidth = static_cast<std::int32_t>(64 * scaleFactor);

					if (pixelHeight != oldPixelHeight || pixelWidth != oldPixelWidth) {

						if (oldPixelHeight > 0 || oldPixelWidth > 0) {

							auto newIcon = winrt::BitmapImage{};
							newIcon.UriSource(icon.UriSource());

							appResources.Insert(iconKey, newIcon);
							icon = newIcon;
						}

						icon.DecodePixelHeight(pixelHeight);
						icon.DecodePixelWidth(pixelWidth);
					}

					args.DragUIOverride().SetContentFromBitmapImage(icon);
					break;
				}
			}
		}
	}

	auto MainWindow::NavView_SelectionChanged(winrt::NavigationView const&, winrt::NavigationViewSelectionChangedEventArgs const& args) -> void {

		auto navItem = args.SelectedItem();
		auto transitionInfo = args.RecommendedNavigationTransitionInfo();

		auto& settings = ::Citrine::ApplicationData::LocalSettings();
		if (navItem == JavaNavItem()) {

			settings.LandingPage("Java");
			frame.Navigate(winrt::xaml_typename<Citrine::MinecraftJavaPage>(), nullptr, transitionInfo);
		}
		else if (navItem == BedrockNavItem()) {

			settings.LandingPage("Bedrock");
			frame.Navigate(winrt::xaml_typename<Citrine::MinecraftBedrockPage>(), nullptr, transitionInfo);
		}
		else {

			frame.Navigate(winrt::xaml_typename<Citrine::SettingsPage>(), nullptr, transitionInfo);
		}
	}

	auto MainWindow::UpdateTitleBarDragArea() -> void {

		auto titleBar = TitleBar();
		auto scaleFactor = static_cast<double>(::GetDpiForWindow(NativeHandle())) / USER_DEFAULT_SCREEN_DPI;

		auto rect = winrt::RectInt32{
			.X = 0,
			.Y = 0,
			.Width = static_cast<std::int32_t>(titleBar.ActualWidth() * scaleFactor),
			.Height = static_cast<std::int32_t>(titleBar.ActualHeight() * scaleFactor)
		};
		AppWindow().TitleBar().SetDragRectangles({ &rect, 1 });
	}

	auto MainWindow::OnWindowMessage(::HWND window, ::UINT messageId, ::WPARAM, ::LPARAM) -> void {

		if (messageId == WM_CLOSE) {

			// pages with NavigationCacheMode.Required cause an access violation if the frame is destroyed in the window destructor
			NavView().Content(nullptr);
			frame = nullptr;

			App::Current().CleanupAndExitAsync();
			return;
		}

		if (messageId == WM_ENDSESSION) {

			App::Current().Cleanup();
			return;
		}

		if (messageId == redirectEventId) {

			::ShowWindow(window, SW_RESTORE);
			::SetForegroundWindow(window);
			return;
		}
	}
}
