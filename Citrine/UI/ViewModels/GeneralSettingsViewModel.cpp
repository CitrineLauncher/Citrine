#include "pch.h"
#include "GeneralSettingsViewModel.h"
#if __has_include("GeneralSettingsViewModel.g.cpp")
#include "GeneralSettingsViewModel.g.cpp"
#endif

#include "ApplicationData.h"
#include "App.xaml.h"
#include "UI/Mvvm/RelayCommand.h"
#include "Locale/Localizer.h"
#include "Windows/Shell.h"

using namespace Citrine;

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
}

namespace winrt::Citrine::implementation
{
	GeneralSettingsViewModel::GeneralSettingsViewModel() {

        auto& supportedLanguages = Localizer::SupportedLanguages;
        auto& settings = ApplicationData::LocalSettings();
      
        auto vec = std::vector<winrt::IInspectable>{};
        vec.reserve(1 + supportedLanguages.size());
        vec.emplace_back(winrt::box_value(Localizer::GetString(L"Language_SystemLanguage")));

        if (settings.Language().IsEmpty())
            requestedLanguage = 0;

        if (Localizer::CurrentLanguage().IsEmpty())
            initialLanguage = 0;

        {
            auto const begin = supportedLanguages.begin();
            auto const end = supportedLanguages.end();
            auto it = begin;

            while (it < end) {

                auto& [name, tag] = *it;

                if (requestedLanguage == -1 && tag == settings.Language())
                    requestedLanguage = 1 + static_cast<std::int32_t>(it - begin);

                if (initialLanguage == -1 && tag == Localizer::CurrentLanguage())
                    initialLanguage = 1 + static_cast<std::int32_t>(it - begin);

                vec.emplace_back(winrt::box_value(name));
                ++it;
            }
        }
        languages = winrt::single_threaded_vector(std::move(vec));

        restartAppCommand = MakeRelayCommand([this] { RestartApp(); });
        openAppDataDirectoryCommand = MakeRelayCommand([this] { OpenAppDataDirectory(); });
	}

    auto GeneralSettingsViewModel::Theme() const noexcept -> std::int32_t {

        auto& settings = ApplicationData::LocalSettings();
        return std::to_underlying(settings.Theme());
    }

    auto GeneralSettingsViewModel::Theme(std::int32_t value) -> void {

        auto& settings = ApplicationData::LocalSettings();
        settings.Theme(static_cast<AppTheme>(value));
    }

    auto GeneralSettingsViewModel::Backdrop() const noexcept -> std::int32_t {

        auto& settings = ApplicationData::LocalSettings();
        return std::to_underlying(settings.Backdrop());
    }

    auto GeneralSettingsViewModel::Backdrop(std::int32_t value) -> void {

        auto& settings = ApplicationData::LocalSettings();
        settings.Backdrop(static_cast<AppBackdrop>(value));
    }

    auto GeneralSettingsViewModel::Languages() const noexcept -> winrt::IVector<winrt::IInspectable> {

        return languages;
    }

    auto GeneralSettingsViewModel::Language() const noexcept -> std::int32_t {

        return requestedLanguage;
    }

    auto GeneralSettingsViewModel::Language(std::int32_t value) -> void {

        if (requestedLanguage == value)
            return;

        auto& supportedLanguages = Localizer::SupportedLanguages;
        auto& settings = ApplicationData::LocalSettings();

        if (value == 0) {

            settings.Language({});
        }
        else {

            auto name = winrt::unbox_value<winrt::hstring>(languages.GetAt(static_cast<std::uint32_t>(value)));
            auto it = supportedLanguages.find(name);
            if (it != supportedLanguages.end()) {

                settings.Language(it->second);
            }
        }

        requestedLanguage = value;
        OnPropertyChanged(languageHasChangedProperty);
    }

    auto GeneralSettingsViewModel::LanguageHasChanged() const noexcept -> bool {

        return requestedLanguage != initialLanguage;
    }

    auto GeneralSettingsViewModel::RestartAppCommand() -> winrt::Microsoft::UI::Xaml::Input::ICommand {

        return restartAppCommand;
    }

    auto GeneralSettingsViewModel::OpenAppDataDirectoryCommand() -> winrt::Microsoft::UI::Xaml::Input::ICommand {

        return openAppDataDirectoryCommand;
    }

    auto GeneralSettingsViewModel::RestartApp() -> void {

        App::Current().Restart();
    }

    auto GeneralSettingsViewModel::OpenAppDataDirectory() -> winrt::fire_and_forget {

        using ::Citrine::Windows::Shell;
        co_await Shell::OpenFolderAsync(ApplicationData::LocalDirectory());
    }

    winrt::hstring const GeneralSettingsViewModel::languageHasChangedProperty = L"LanguageHasChanged";
}
