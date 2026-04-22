#pragma once

#include "Locale/LanguageTag.h"
#include "Core/Storage/JsonStorage.h"
#include "Core/Util/Event.h"

#include <string>

#include <glaze/json.hpp>

namespace Citrine {

	enum struct AppTheme {

		System,
		Light,
		Dark
	};

	enum struct AppBackdrop {

		Mica,
		MicaAlt
	};

	class LocalApplicationSettings {
	public:

		using ThemeChangedEventHandler = EventHandler<AppTheme>;
		using BackdropChangedEventHandler = EventHandler<AppBackdrop>;

		auto Theme() const noexcept -> AppTheme;
		auto Theme(AppTheme value) -> void;

		auto ThemeChanged(ThemeChangedEventHandler handler) -> EventToken;
		auto ThemeChanged(EventToken&& token) -> void;

		auto Backdrop() const noexcept -> AppBackdrop;
		auto Backdrop(AppBackdrop value) -> void;

		auto BackdropChanged(BackdropChangedEventHandler handler) -> EventToken;
		auto BackdropChanged(EventToken&& token) -> void;

		auto Language() const noexcept -> LanguageTag const&;
		auto Language(LanguageTag const& value) noexcept -> void;

		auto LandingPage() const noexcept -> std::string const&;
		auto LandingPage(std::string value) noexcept -> void;

		auto Save() -> StorageOperationResult;
		auto Close() -> void;

	private:

		constexpr LocalApplicationSettings() = default;

		LocalApplicationSettings(LocalApplicationSettings const&) = delete;
		auto operator=(LocalApplicationSettings const&) = delete;

		auto Initialize(std::filesystem::path const& path) -> void;

		friend class ApplicationData;

		struct SettingsData {

			AppTheme Theme{};
			AppBackdrop Backdrop{};
			LanguageTag Language;
			std::string LandingPage;
		};

		friend struct ::glz::meta<SettingsData>;

		JsonStorage<SettingsData> storage;
		Event<ThemeChangedEventHandler> themeChangedEvent;
		Event<BackdropChangedEventHandler> backdropChangedEvent;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::AppTheme> {

		using enum ::Citrine::AppTheme;

		static constexpr auto value = enumerate(
			"System", System,
			"Light", Light,
			"Dark", Dark
		);
	};

	template<>
	struct meta<::Citrine::AppBackdrop> {

		using enum ::Citrine::AppBackdrop;

		static constexpr auto value = enumerate(
			"Mica", Mica,
			"MicaAlt", MicaAlt
		);
	};

	template<>
	struct meta<::Citrine::LocalApplicationSettings::SettingsData> {

		using T = ::Citrine::LocalApplicationSettings::SettingsData;

		static constexpr auto value = object(
			"Theme", &T::Theme,
			"Backdrop", &T::Backdrop,
			"Language", &T::Language,
			"LandingPage", &T::LandingPage
		);
	};
}