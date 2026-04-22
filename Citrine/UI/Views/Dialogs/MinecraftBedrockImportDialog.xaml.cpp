#include "pch.h"
#include "MinecraftBedrockImportDialog.xaml.h"
#if __has_include("MinecraftBedrockImportDialog.g.cpp")
#include "MinecraftBedrockImportDialog.g.cpp"
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
	MinecraftBedrockImportDialog::MinecraftBedrockImportDialog(Citrine::MinecraftBedrockGamePackagesViewModel const& viewModel, winrt::hstring const& gamePackageLocation)

		: viewModel(viewModel.as<implementation::MinecraftBedrockGamePackagesViewModel>())
	{
		EnsureProperties();

		InitiateImport(gamePackageLocation);
		Opened([this](auto const&...) {

			if (!initiateImportOp && !importContext)
				Hide();
		});

		CloseButtonText(Localizer::GetString(L"Action_Cancel"));
		CloseButtonClick([this](auto const&...) {

			if (initiateImportOp)
				initiateImportOp.Cancel();
		});
	}

	auto MinecraftBedrockImportDialog::ViewModel() const noexcept -> Citrine::MinecraftBedrockGamePackagesViewModel {

		return *viewModel;
	}

	auto MinecraftBedrockImportDialog::GamePackage() const -> Citrine::MinecraftBedrockGamePackageItem {

		return GetValue(gamePackageProperty).try_as<Citrine::MinecraftBedrockGamePackageItem>();
	}

	auto MinecraftBedrockImportDialog::GamePackageProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty {

		return gamePackageProperty;
	}

	auto MinecraftBedrockImportDialog::PickInstallLocationButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const& args) -> void {

		auto picker = winrt::make_self<implementation::MinecraftBedrockInstallLocationPicker>();
		picker->XamlRoot(XamlRoot());
		picker->DefaultInstallLocation(viewModel->CurrentInstallLocation());

		auto installLocation = picker->PickInstallLocation();
		if (!installLocation.empty())
			InstallLocationTextBox().Text(installLocation);
	}

	auto MinecraftBedrockImportDialog::OnPrimaryButtonClick(winrt::ContentDialog const&, winrt::ContentDialogButtonClickEventArgs const& args) -> void {

		if (!importContext)
			return;

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

		viewModel->ImportGamePackage(*importContext, installLocation, NameTagText().Text());
		primaryButtonClickRevoker.revoke();
	}

	auto MinecraftBedrockImportDialog::EnsureProperties() -> void {

		if (!gamePackageProperty) {

			gamePackageProperty = winrt::DependencyProperty::Register(
				L"GamePackageProperty",
				winrt::xaml_typename<Citrine::MinecraftBedrockGamePackageItem>(),
				winrt::xaml_typename<class_type>(),
				nullptr
			);
		}
	}

	auto MinecraftBedrockImportDialog::InitiateImport(winrt::hstring const& gamePackageLocation) -> winrt::fire_and_forget {

		initiateImportOp = viewModel->InitiateGamePackageImport(gamePackageLocation);
		try {

			auto weakSelf = get_weak();
			auto result = co_await initiateImportOp;

			if (auto self = weakSelf.get()) {

				initiateImportOp = nullptr;
				importContext = result.try_as<implementation::MinecraftBedrockGamePackageImportContext>();

				if (!importContext) {

					self->Hide();
					co_return;
				}

				SetValue(gamePackageProperty, importContext->GamePackage());

				PrimaryButtonText(Localizer::GetString(L"Action_Import"));
				DefaultButton(winrt::ContentDialogButton::Primary);

				primaryButtonClickRevoker = PrimaryButtonClick(winrt::auto_revoke, { this, &MinecraftBedrockImportDialog::OnPrimaryButtonClick });
			}
		}
		catch (...) {}
	}

	winrt::DependencyProperty MinecraftBedrockImportDialog::gamePackageProperty{ nullptr };
}
