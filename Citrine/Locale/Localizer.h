#pragma once

#include "Locale/LanguageTag.h"
#include "Core/Util/Frozen.h"

#include <string_view>

namespace Citrine {

	class Localizer {
	public:

		static constexpr auto SupportedLanguages = MakeFrozenMap<std::wstring_view, LanguageTag>({

			{ L"Deutsch", "de" },
			{ L"English", "en" },
		});

		static auto Initialize(LanguageTag const& language = {}) -> void;
		static auto CurrentLanguage() -> LanguageTag const&;
		static auto GetString(std::wstring_view id) -> winrt::hstring;
	};
}
