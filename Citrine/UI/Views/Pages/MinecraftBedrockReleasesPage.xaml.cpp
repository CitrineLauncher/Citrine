#include "pch.h"
#include "MinecraftBedrockReleasesPage.xaml.h"
#if __has_include("MinecraftBedrockReleasesPage.g.cpp")
#include "MinecraftBedrockReleasesPage.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml::Markup;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	MinecraftBedrockReleasesPage::MinecraftBedrockReleasesPage()

		: MinecraftBedrockReleasesPageT(L"Releases")
	{}

	auto MinecraftBedrockReleasesPage::InitializeComponent() -> void {

		composable_base::InitializeComponent();
	}

	auto MinecraftBedrockReleasesPage::Connect(std::int32_t connectionId, winrt::IInspectable const& target) -> void {

		composable_base::Connect(connectionId, target);
	}

	auto MinecraftBedrockReleasesPage::GetBindingConnector(std::int32_t connectionId, winrt::IInspectable const& target) -> winrt::IComponentConnector {

		return composable_base::GetBindingConnector(connectionId, target);
	}
}
