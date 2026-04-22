#include "pch.h"
#include "Localizer.h"

#include <winrt/Microsoft.Windows.Globalization.h>
#include <winrt/Microsoft.Windows.ApplicationModel.Resources.h>

using namespace Citrine;

namespace winrt {

	using namespace Microsoft::Windows::Globalization;
	using namespace Microsoft::Windows::ApplicationModel::Resources;
}

namespace {

	auto resourceLoader = winrt::ResourceLoader{ nullptr };
	auto currentLanguage = LanguageTag{};
}

namespace Citrine {

	auto Localizer::Initialize(LanguageTag const& language) -> void {

		if (!language.IsEmpty()) try {

			winrt::ApplicationLanguages::PrimaryLanguageOverride(winrt::to_hstring(language));
			currentLanguage = language;
		}
		catch (...) {}
		resourceLoader = winrt::ResourceLoader{};
	}

	auto Localizer::CurrentLanguage() -> LanguageTag const& {

		return currentLanguage;
	}

	auto Localizer::GetString(std::wstring_view id) -> winrt::hstring {

		return resourceLoader.GetString(id);
	}
}