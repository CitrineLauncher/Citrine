#include "pch.h"
#include "MinecraftBedrockPreviewsPage.xaml.h"
#if __has_include("MinecraftBedrockPreviewsPage.g.cpp")
#include "MinecraftBedrockPreviewsPage.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml::Markup;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	MinecraftBedrockPreviewsPage::MinecraftBedrockPreviewsPage()

		: MinecraftBedrockPreviewsPageT(L"Previews")
	{}

	auto MinecraftBedrockPreviewsPage::InitializeComponent() -> void {

		composable_base::InitializeComponent();
	}

	auto MinecraftBedrockPreviewsPage::Connect(std::int32_t connectionId, winrt::IInspectable const& target) -> void {

		composable_base::Connect(connectionId, target);
	}

	auto MinecraftBedrockPreviewsPage::GetBindingConnector(std::int32_t connectionId, winrt::IInspectable const& target) -> winrt::IComponentConnector {

		return composable_base::GetBindingConnector(connectionId, target);
	}
}
