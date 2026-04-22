#pragma once

#include "Models/MinecraftBedrockGamePackageItem.h"

namespace winrt::Citrine::implementation {

	class MinecraftBedrockGamePackageViewBase {
	public:

		auto GetPackageIconSource(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::Microsoft::UI::Xaml::Media::ImageSource;
		auto GetPackageTitleString(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::hstring;
		auto GetPackageTitleString(Citrine::MinecraftBedrockGamePackageItem const& item, winrt::hstring const& nameTag) const noexcept -> winrt::hstring;
		auto GetNameTagLength(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> std::int32_t;
		auto GetPackageDescriptionString(Citrine::MinecraftBedrockGamePackageItem const& item) const noexcept -> winrt::hstring;
		auto GetPackageStatusString(Citrine::MinecraftBedrockGamePackageStatus status) const noexcept -> winrt::hstring;
		auto GetOperationProgressPercentage(Citrine::ByteProgress const& progress) const noexcept -> double;
		auto GetOperationProgressString(Citrine::ByteProgress const& progress) const noexcept -> winrt::hstring;

	protected:

		MinecraftBedrockGamePackageViewBase();
	};
}
