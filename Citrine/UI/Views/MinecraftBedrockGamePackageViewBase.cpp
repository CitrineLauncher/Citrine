#include "pch.h"
#include "MinecraftBedrockGamePackageViewBase.h"

#include "App.xaml.h";
#include "Locale/Localizer.h"
#include "Helpers/ByteProgressFormatter.h"

#include <optional>
#include <string>
#include <algorithm>

#include <winstring.h>
#pragma comment(lib, "windowsapp.lib")

using namespace Citrine;
using namespace Minecraft::Bedrock;
using namespace Windows;

namespace winrt {

	using namespace Microsoft::UI::Xaml::Media;
}

namespace {

	using winrt::Citrine::implementation::App;
	using winrt::Citrine::ByteProgress;
	using winrt::Citrine::ByteProgressFormatter;
	using GamePackageItem = winrt::Citrine::MinecraftBedrockGamePackageItem;
	using GamePackageItemImpl = winrt::Citrine::implementation::MinecraftBedrockGamePackageItem;
	using GamePackageStatus = winrt::Citrine::MinecraftBedrockGamePackageStatus;

	struct IconResources {

		IconResources() {

			auto appResources = App::Current().Resources();
			Default = appResources.Lookup(winrt::box_value(L"MinecraftBedrockIcon")).as<winrt::ImageSource>();
			Preview = appResources.Lookup(winrt::box_value(L"MinecraftBedrockPreviewIcon")).as<winrt::ImageSource>();
		}

		winrt::ImageSource Default{ nullptr };
		winrt::ImageSource Preview{ nullptr };
	};

	struct StringResources {

		StringResources() {

			{
				auto combine = [buffer = std::wstring{}](auto const& buildType, auto const& platform) mutable {

					auto combinedSize = buildType.size() + 1 + platform.size();
					buffer.resize_and_overwrite(combinedSize, [&](wchar_t* data, std::size_t) {

						auto out = data;
						out = std::ranges::copy(buildType, out).out;
						*out++ = L'\uFE31';
						out = std::ranges::copy(platform, out).out;
						return out - data;
					});
					return winrt::hstring{ buffer };
				};

				auto uwp = Localizer::GetString(L"Platform_UWP");
				auto gdk = Localizer::GetString(L"Platform_GDK");

				Release = Localizer::GetString(L"BuildType_Release");
				Release_UWP = combine(Release, uwp);
				Release_GDK = combine(Release, gdk);

				Preview = Localizer::GetString(L"BuildType_Preview");
				Preview_UWP = combine(Preview, uwp);
				Preview_GDK = combine(Preview, gdk);

				Beta = Localizer::GetString(L"BuildType_Beta");
				Beta_UWP = combine(Beta, uwp);
				Beta_GDK = combine(Beta, gdk);
			}

			RatioDelimiter = Localizer::GetString(L"RatioDelimiterString");

			NotInstalled			= Localizer::GetString(L"Status_NotInstalled");
			PreparingDownload		= Localizer::GetString(L"Status_PreparingDownload");
			Downloading				= Localizer::GetString(L"Status_Downloading");
			Extracting				= Localizer::GetString(L"Status_Extracting");
			CancellingInstallation	= Localizer::GetString(L"Status_CancellingInstallation");
			InstallationPaused		= Localizer::GetString(L"Status_InstallationPaused");
			InstallationFailed		= Localizer::GetString(L"Status_InstallationFailed");
			Installed				= Localizer::GetString(L"Status_Installed");
			PreparingRepair			= Localizer::GetString(L"Status_PreparingRepair");
			Repairing				= Localizer::GetString(L"Status_Repairing");
			RepairFailed			= Localizer::GetString(L"Status_RepairFailed");
			Registering				= Localizer::GetString(L"Status_Registering");
			Launching				= Localizer::GetString(L"Status_Launching");
			Uninstalling			= Localizer::GetString(L"Status_Uninstalling");
			UninstallationPending	= Localizer::GetString(L"Status_UninstallationPending");
			UninstallationFailed	= Localizer::GetString(L"Status_UninstallationFailed");
		}

		winrt::hstring Release;
		winrt::hstring Release_UWP;
		winrt::hstring Release_GDK;
		winrt::hstring Preview;
		winrt::hstring Preview_UWP;
		winrt::hstring Preview_GDK;
		winrt::hstring Beta;
		winrt::hstring Beta_UWP;
		winrt::hstring Beta_GDK;
		winrt::hstring RatioDelimiter;
		winrt::hstring NotInstalled;
		winrt::hstring PreparingDownload;
		winrt::hstring Downloading;
		winrt::hstring Extracting;
		winrt::hstring CancellingInstallation;
		winrt::hstring InstallationPaused;
		winrt::hstring InstallationFailed;
		winrt::hstring Installed;
		winrt::hstring PreparingRepair;
		winrt::hstring Repairing;
		winrt::hstring RepairFailed;
		winrt::hstring Registering;
		winrt::hstring Launching;
		winrt::hstring Uninstalling;
		winrt::hstring UninstallationPending;
		winrt::hstring UninstallationFailed;
	};

	struct Resources {

		IconResources Icons;
		StringResources Strings;
	};

	auto resources = std::optional<Resources>{};
}

namespace winrt::Citrine::implementation {

	MinecraftBedrockGamePackageViewBase::MinecraftBedrockGamePackageViewBase() {

		if (!resources)
			resources.emplace();
	}

	auto MinecraftBedrockGamePackageViewBase::GetPackageIconSource(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::ImageSource {

		if (!item)
			return nullptr;

		auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
		auto& id = itemImpl->Id();
		
		auto& icons = resources->Icons;
		return (id.BuildType == GameBuildType::Preview)
			? icons.Preview
			: icons.Default;
	}

	auto MinecraftBedrockGamePackageViewBase::GetPackageTitleString(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::hstring {

		if (!item)
			return {};

		auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
		auto nameTag = itemImpl->NameTag();

		return GetPackageTitleString(item, nameTag);
	}

	auto MinecraftBedrockGamePackageViewBase::GetPackageTitleString(Citrine::MinecraftBedrockGamePackageItem const& item, winrt::hstring const& nameTag) const noexcept -> winrt::hstring {

		if (!item)
			return {};

		auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
		auto version = itemImpl->NormalizedVersion();

		if (nameTag.empty())
			return version;

		auto bufferSize = version.size() + 1 + nameTag.size();
		auto bufferData = static_cast<wchar_t*>(nullptr);

		auto bufferHandle = ::HSTRING_BUFFER{};
		if (::WindowsPreallocateStringBuffer(bufferSize, &bufferData, &bufferHandle) != S_OK)
			return {};

		auto out = bufferData;
		out = std::ranges::copy(version, out).out;
		*out++ = L'\u2004';
		out = std::ranges::copy(nameTag, out).out;

		auto stringHandle = ::HSTRING{};
		::WindowsPromoteStringBuffer(bufferHandle, &stringHandle);
		return winrt::hstring{ stringHandle, winrt::take_ownership_from_abi };
	}

	auto MinecraftBedrockGamePackageViewBase::GetNameTagLength(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> std::int32_t {

		if (!item)
			return 0;

		auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
		return itemImpl->NameTag().size();
	}

	auto MinecraftBedrockGamePackageViewBase::GetPackageDescriptionString(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::hstring {

		if (!item)
			return {};

		auto itemImpl = winrt::get_self<GamePackageItemImpl>(item);
		auto& id = itemImpl->Id();

#if defined(_M_X64)
		auto nativeArch = id.Architecture == PackageArchitecture::X64;
#elif
		auto nativeArch = false;
#endif

		auto& strings = resources->Strings;
		auto description = winrt::hstring{};

		using enum GameBuildType;
		switch (id.BuildType) {
		case Release: {

			using enum GamePlatform;
			switch (id.Platform) {
			case XboxUWP:
				[[fallthrough]];
			case WindowsUWP:
				description = strings.Release_UWP;
				break;
			case WindowsGDK: description = nativeArch
				? strings.Release
				: strings.Release_GDK;
				break;
			}
		} break;
		case Preview: {

			using enum GamePlatform;
			switch (id.Platform) {
			case XboxUWP:
				[[fallthrough]];
			case WindowsUWP:
				description = strings.Preview_UWP;
				break;
			case WindowsGDK: description = nativeArch
				? strings.Preview
				: strings.Preview_GDK;
				break;
			}
		} break;
		case Beta: {

			using enum GamePlatform;
			switch (id.Platform) {
			case XboxUWP:
				[[fallthrough]];
			case WindowsUWP:
				description = strings.Beta_UWP;
				break;
			case WindowsGDK: description = nativeArch
				? strings.Beta
				: strings.Beta_GDK;
				break;
			}
		} break;
		}

		if (!nativeArch) {

			auto archName = id.Architecture.Name();
			auto bufferSize = description.size() + 2 + archName.size() + 1;
			auto bufferData = static_cast<wchar_t*>(nullptr);

			auto bufferHandle = ::HSTRING_BUFFER{};
			if (::WindowsPreallocateStringBuffer(bufferSize, &bufferData, &bufferHandle) != S_OK)
				return {};

			auto out = bufferData;
			out = std::ranges::copy(description, out).out;
			*out++ = L' ';
			*out++ = L'(';
			out = std::ranges::copy(archName, out).out;
			*out++ = L')';

			auto stringHandle = ::HSTRING{};
			::WindowsPromoteStringBuffer(bufferHandle, &stringHandle);
			description = winrt::hstring{ stringHandle, winrt::take_ownership_from_abi };
		}

		return description;
	}

	auto MinecraftBedrockGamePackageViewBase::GetPackageStatusString(Citrine::MinecraftBedrockGamePackageStatus status) const noexcept -> winrt::hstring {

		auto& strings = resources->Strings;
		auto statusStr = winrt::hstring{};

		using enum GamePackageStatus;
		switch (status) {
		case NotInstalled:				statusStr = strings.NotInstalled;			break;
		case PreparingDownload:			statusStr = strings.PreparingDownload;		break;
		case Downloading:				statusStr = strings.Downloading;			break;
		case Extracting:				statusStr = strings.Extracting;				break;
		case CancellingInstallation:	statusStr = strings.CancellingInstallation;	break;
		case InstallationPaused:		statusStr = strings.InstallationPaused;		break;
		case InstallationFailed:		statusStr = strings.InstallationFailed;		break;
		case Installed:					statusStr = strings.Installed;				break;
		case PreparingRepair:			statusStr = strings.PreparingRepair;		break;
		case Repairing:					statusStr = strings.Repairing;				break;
		case RepairFailed:				statusStr = strings.RepairFailed;			break;
		case Registering:				statusStr = strings.Registering;			break;
		case Launching:					statusStr = strings.Launching;				break;
		case Uninstalling:				statusStr = strings.Uninstalling;			break;
		case UninstallationPending:		statusStr = strings.UninstallationPending;	break;
		case UninstallationFailed:		statusStr = strings.UninstallationFailed;	break;
		}

		return statusStr;
	}

	auto MinecraftBedrockGamePackageViewBase::GetOperationProgressPercentage(Citrine::ByteProgress const& progress) const noexcept -> double {

		return progress.TotalBytesToProcess > 0
			? static_cast<double>(progress.BytesProcessed) / static_cast<double>(progress.TotalBytesToProcess) * 100
			: 0.0;
	}

	auto MinecraftBedrockGamePackageViewBase::GetOperationProgressString(Citrine::ByteProgress const& progress) const noexcept -> winrt::hstring {

		if (progress == Citrine::ByteProgress{})
			return {};
		return ByteProgressFormatter::Format(progress, resources->Strings.RatioDelimiter);
	}
}