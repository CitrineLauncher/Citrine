#include "pch.h"
#include "MinecraftBedrockImportsPage.xaml.h"
#if __has_include("MinecraftBedrockImportsPage.g.cpp")
#include "MinecraftBedrockImportsPage.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml::Markup;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	MinecraftBedrockImportsPage::MinecraftBedrockImportsPage()

		: MinecraftBedrockImportsPageT(L"Imports")
	{}

	auto MinecraftBedrockImportsPage::InitializeComponent() -> void {

		composable_base::InitializeComponent();
	}

	auto MinecraftBedrockImportsPage::Connect(std::int32_t connectionId, winrt::IInspectable const& target) -> void {

		composable_base::Connect(connectionId, target);
	}

	auto MinecraftBedrockImportsPage::GetBindingConnector(std::int32_t connectionId, winrt::IInspectable const& target) -> winrt::IComponentConnector {

		return composable_base::GetBindingConnector(connectionId, target);
	}
}
