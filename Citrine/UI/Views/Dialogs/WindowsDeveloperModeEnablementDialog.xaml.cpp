#include "pch.h"
#include "WindowsDeveloperModeEnablementDialog.xaml.h"
#if __has_include("WindowsDeveloperModeEnablementDialog.g.cpp")
#include "WindowsDeveloperModeEnablementDialog.g.cpp"
#endif

#include "Locale/Localizer.h"
#include "Core/Util/Scope.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
}

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
	WindowsDeveloperModeEnablementDialog::WindowsDeveloperModeEnablementDialog(Citrine::WindowsDeveloperModeEnablementViewModel const& viewModel)
		
		: viewModel(viewModel.as<implementation::WindowsDeveloperModeEnablementViewModel>())
	{
		Title(winrt::box_value(Localizer::GetString(L"Enablement_WindowsDeveloperMode_DialogTitle")));
		Content(winrt::box_value(Localizer::GetString(L"Enablement_WindowsDeveloperMode_DialogContent")));

		Closing([this](auto const&...) {

			if (enableOp)
				enableOp.Cancel();
		});

		PrimaryButtonText(Localizer::GetString(L"Action_Enable"));
		CloseButtonText(Localizer::GetString(L"Action_Cancel"));
		DefaultButton(winrt::ContentDialogButton::Primary);

		primaryButtonClickRevoker = PrimaryButtonClick(winrt::auto_revoke, { this, &WindowsDeveloperModeEnablementDialog::OnPrimaryButtonClick });
	}

	auto WindowsDeveloperModeEnablementDialog::ViewModel() const noexcept -> Citrine::WindowsDeveloperModeEnablementViewModel {

		return *viewModel;
	}

	auto WindowsDeveloperModeEnablementDialog::OnPrimaryButtonClick(winrt::ContentDialog const&, winrt::ContentDialogButtonClickEventArgs const& args) -> winrt::fire_and_forget {

		if (enableOp) {

			args.Cancel(true);
			co_return;
		}

		enableOp = viewModel->EnableWindowsDeveloperMode();
		primaryButtonClickRevoker.revoke();

		auto defferal = args.GetDeferral();
		auto completeDefferal = ScopeExit{ [&] { defferal.Complete(); } };

		try {

			auto weakSelf = get_weak();
			co_await enableOp;

			if (auto self = weakSelf.get()) {

				enableOp = nullptr;
			}
		}
		catch (...) {}
	}
}
