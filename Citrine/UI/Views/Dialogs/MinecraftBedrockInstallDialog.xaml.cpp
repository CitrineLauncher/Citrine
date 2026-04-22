#include "pch.h"
#include "MinecraftBedrockInstallDialog.xaml.h"
#if __has_include("MinecraftBedrockInstallDialog.g.cpp")
#include "MinecraftBedrockInstallDialog.g.cpp"
#endif

#include "Locale/Localizer.h"
#include "UI/Views/Dialogs/MinecraftBedrockInstallLocationPicker.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
}

namespace winrt::Citrine::implementation
{
	MinecraftBedrockInstallDialog::MinecraftBedrockInstallDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage)

		: viewModel(viewModel.as<implementation::MinecraftBedrockGamePackagesViewModel>())
		, gamePackage(gamePackage)
	{
		PrimaryButtonText(Localizer::GetString(L"Action_Install"));
		CloseButtonText(Localizer::GetString(L"Action_Cancel"));
		DefaultButton(winrt::ContentDialogButton::Primary);

		primaryButtonClickRevoker = PrimaryButtonClick(winrt::auto_revoke, { this, &MinecraftBedrockInstallDialog::OnPrimaryButtonClick });
	}

	auto MinecraftBedrockInstallDialog::ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockInstallDialog::GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

		return gamePackage;
	}

	auto MinecraftBedrockInstallDialog::PickInstallLocationButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const& args) -> void {

		auto picker = winrt::make_self<implementation::MinecraftBedrockInstallLocationPicker>();
		picker->XamlRoot(XamlRoot());
		picker->DefaultInstallLocation(viewModel->CurrentInstallLocation());

		auto installLocation = picker->PickInstallLocation();
		if (!installLocation.empty())
			InstallLocationTextBox().Text(installLocation);
	}

	auto MinecraftBedrockInstallDialog::OnPrimaryButtonClick(winrt::ContentDialog const&, winrt::ContentDialogButtonClickEventArgs const& args) -> void {

		using enum InstallLocationValidationResult;

		auto installLocation = InstallLocationTextBox().Text();
		auto validationResult = viewModel->ValidateInstallLocation(installLocation);

		if (validationResult != Success) {

			auto id = std::wstring_view{};
			switch (validationResult) {
			case InvalidPath:			id = L"ErrorMessage_InvalidPath";			break;
			case MaxPathLengthExceeded:	id = L"ErrorMessage_MaxPathLengthExceeded";	break;
			case DirectoryNotFound:		id = L"ErrorMessage_DirectoryNotFound";		break;
			default:					id = L"ErrorMessage_Unknown";				break;
			}

			InstallLocationErrorText().Text(Localizer::GetString(id));
			args.Cancel(true);
			return;
		}

		viewModel->InstallGamePackage(gamePackage, installLocation);
		primaryButtonClickRevoker.revoke();
	}
}
