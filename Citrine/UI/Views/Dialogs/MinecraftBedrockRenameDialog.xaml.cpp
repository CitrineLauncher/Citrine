#include "pch.h"
#include "MinecraftBedrockRenameDialog.xaml.h"
#if __has_include("MinecraftBedrockRenameDialog.g.cpp")
#include "MinecraftBedrockRenameDialog.g.cpp"
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
	MinecraftBedrockRenameDialog::MinecraftBedrockRenameDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, Citrine::MinecraftBedrockGamePackageItem const& gamePackage)

		: viewModel(viewModel.as<implementation::MinecraftBedrockGamePackagesViewModel>())
		, gamePackage(gamePackage)
	{
		PrimaryButtonText(Localizer::GetString(L"Action_Save"));
		CloseButtonText(Localizer::GetString(L"Action_Cancel"));
		DefaultButton(winrt::ContentDialogButton::Primary);

		primaryButtonClickRevoker = PrimaryButtonClick(winrt::auto_revoke, { this, &MinecraftBedrockRenameDialog::OnPrimaryButtonClick });
	}

	auto MinecraftBedrockRenameDialog::ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockRenameDialog::GamePackage() const noexcept -> Citrine::MinecraftBedrockGamePackageItem {

		return gamePackage;
	}

	auto MinecraftBedrockRenameDialog::OnPrimaryButtonClick(winrt::ContentDialog const&, winrt::ContentDialogButtonClickEventArgs const& args) -> void {

		viewModel->RenameGamePackage(gamePackage, NameTagText().Text());
		primaryButtonClickRevoker.revoke();
	}
}
