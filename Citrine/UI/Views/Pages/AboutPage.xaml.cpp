#include "pch.h"
#include "AboutPage.xaml.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	auto AboutPage::ViewModel() const noexcept -> Citrine::AboutViewModel {

		return *viewModel;
	}
}
