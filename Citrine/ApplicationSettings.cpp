#include "pch.h"
#include "ApplicationSettings.h"

namespace Citrine {

	auto LocalApplicationSettings::Initialize(std::filesystem::path const& path) -> void {

		storage.Open(path);

		if (!storage.Load())
			storage.Save();
	}

	auto LocalApplicationSettings::Theme() const noexcept -> AppTheme {

		return storage->Theme;
	}

	auto LocalApplicationSettings::Theme(AppTheme value) -> void {

		if (storage->Theme != value) {

			storage->Theme = value;
			themeChangedEvent(value);
		}
	}

	auto LocalApplicationSettings::ThemeChanged(ThemeChangedEventHandler handler) -> EventToken {

		return themeChangedEvent.Add(std::move(handler));
	}

	auto LocalApplicationSettings::ThemeChanged(EventToken&& token) -> void {

		themeChangedEvent.Remove(std::move(token));
	}

	auto LocalApplicationSettings::Backdrop() const noexcept -> AppBackdrop {

		return storage->Backdrop;
	}

	auto LocalApplicationSettings::Backdrop(AppBackdrop value) -> void {

		if (storage->Backdrop != value) {

			storage->Backdrop = value;
			backdropChangedEvent(value);
		}
	}

	auto LocalApplicationSettings::BackdropChanged(BackdropChangedEventHandler handler) -> EventToken {

		return backdropChangedEvent.Add(std::move(handler));
	}

	auto LocalApplicationSettings::BackdropChanged(EventToken&& token) -> void {

		backdropChangedEvent.Remove(std::move(token));
	}

	auto LocalApplicationSettings::Language() const noexcept -> LanguageTag const& {

		return storage->Language;
	}

	auto LocalApplicationSettings::Language(LanguageTag const& value) noexcept -> void {

		storage->Language = value;
	}

	auto LocalApplicationSettings::LandingPage() const noexcept -> std::string const& {

		return storage->LandingPage;
	}

	auto LocalApplicationSettings::LandingPage(std::string value) noexcept -> void {

		storage->LandingPage = std::move(value);
	}

	auto LocalApplicationSettings::Save() -> StorageOperationResult {

		return storage.Save();
	}

	auto LocalApplicationSettings::Close() -> void {

		storage.Close();
	}
}