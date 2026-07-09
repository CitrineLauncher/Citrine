#pragma once

#include "Locale/LanguageTag.h"
#include "Core/Storage/JsonStorage.h"
#include "Core/Util/Event.h"

#include <string>

#include <glaze/json.hpp>

namespace Citrine {

	enum struct AppTheme : std::uint8_t {

		System,
		Light,
		Dark
	};

	enum struct AppBackdrop : std::uint8_t {

		Mica,
		MicaAlt
	};

	enum struct PackageViewMode : std::uint8_t {

		All,
		Installed
	};

	class LocalApplicationSettings {
	public:

		using ThemeChangedEventHandler = EventHandler<AppTheme>;
		using BackdropChangedEventHandler = EventHandler<AppBackdrop>;
		using PackageViewModeChangedEventHandler = EventHandler<Citrine::PackageViewMode>;

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

		auto PackageViewMode() const noexcept -> Citrine::PackageViewMode;
		auto PackageViewMode(Citrine::PackageViewMode value) -> void;

		auto PackageViewModeChanged(PackageViewModeChangedEventHandler handler) -> EventToken;
		auto PackageViewModeChanged(EventToken&& token) -> void;

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
			Citrine::PackageViewMode PackageViewMode{};
			std::string LandingPage;
		};

		friend struct ::glz::meta<SettingsData>;

		JsonStorage<SettingsData> storage;
		Event<ThemeChangedEventHandler> themeChangedEvent;
		Event<BackdropChangedEventHandler> backdropChangedEvent;
		Event<PackageViewModeChangedEventHandler> packageViewModeChangedEvent;
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
	struct meta<::Citrine::PackageViewMode> {

		using enum ::Citrine::PackageViewMode;

		static constexpr auto value = enumerate(
			"All", All,
			"Installed", Installed
		);
	};

	template<>
	struct meta<::Citrine::LocalApplicationSettings::SettingsData> {

		using T = ::Citrine::LocalApplicationSettings::SettingsData;

		static constexpr auto value = object(
			"Theme", &T::Theme,
			"Backdrop", &T::Backdrop,
			"Language", &T::Language,
			"PackageViewMode", &T::PackageViewMode,
			"LandingPage", &T::LandingPage
		);
	};
}