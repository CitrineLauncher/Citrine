#include "pch.h"
#include "MinecraftBedrockManageDialog.xaml.h"
#if __has_include("MinecraftBedrockManageDialog.g.cpp")
#include "MinecraftBedrockManageDialog.g.cpp"
#endif

#include "Locale/Localizer.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
	using namespace Microsoft::UI::Xaml::Controls;
}

namespace winrt::Citrine::implementation
{
	MinecraftBedrockManageDialog::MinecraftBedrockManageDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage)

		: viewModel(viewModel.as<implementation::MinecraftBedrockGamePackagesViewModel>())
		, gamePackage(gamePackage)
	{}

	auto MinecraftBedrockManageDialog::ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockManageDialog::GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

		return gamePackage;
	}

	auto MinecraftBedrockManageDialog::CloseButton2_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const& args) -> void {

		Hide();
	}

	auto MinecraftBedrockManageDialog::OpenGameDirectoryButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const& args) -> void {

		viewModel->OpenGameDirectory(gamePackage);
	}

	auto MinecraftBedrockManageDialog::UninstallButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const& args) -> void {

		CloseButton2().Visibility(winrt::Visibility::Collapsed);
		OptionsArea().Visibility(winrt::Visibility::Collapsed);

		PrimaryButtonText(Localizer::GetString(L"Action_Uninstall"));
		CloseButtonText(Localizer::GetString(L"Action_Cancel"));
		DefaultButton(winrt::ContentDialogButton::Close);

		ConfirmUninstallMessage().Title(Localizer::GetString(L"MinecraftBedrock_GamePackage_ConfirmUninstallMessage"));

		primaryButtonClickRevoker = PrimaryButtonClick(winrt::auto_revoke, [&](auto const&...) {

			viewModel->UninstallGamePackage(gamePackage);
			primaryButtonClickRevoker.revoke();
		});
	}
}
