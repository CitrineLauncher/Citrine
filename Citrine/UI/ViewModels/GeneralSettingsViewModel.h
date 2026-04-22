#pragma once

#include "GeneralSettingsViewModel.g.h"

#include "Helpers/NotifyPropertyChangedBase.h"

namespace winrt::Citrine::implementation
{
    struct GeneralSettingsViewModel : GeneralSettingsViewModelT<GeneralSettingsViewModel>, ::Citrine::NotifyPropertyChangedBase
    {
        GeneralSettingsViewModel();

        auto Theme() const noexcept -> std::int32_t;
        auto Theme(std::int32_t value) -> void;

        auto Backdrop() const noexcept -> std::int32_t;
        auto Backdrop(std::int32_t value) -> void;

        auto Languages() const noexcept -> winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable>;

        auto Language() const noexcept -> std::int32_t;
        auto Language(std::int32_t value) -> void;

        auto LanguageHasChanged() const noexcept -> bool;

        auto RestartAppCommand() -> winrt::Microsoft::UI::Xaml::Input::ICommand;
        auto OpenAppDataDirectoryCommand() -> winrt::Microsoft::UI::Xaml::Input::ICommand;

    private:

        auto RestartApp() -> void;
        auto OpenAppDataDirectory() -> winrt::fire_and_forget;

        static winrt::hstring const languageHasChangedProperty;

        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable> languages{ nullptr };
        std::int32_t requestedLanguage{ -1 };
        std::int32_t initialLanguage{ -1 };
        Citrine::IRelayCommand restartAppCommand{ nullptr };
        Citrine::IRelayCommand openAppDataDirectoryCommand{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct GeneralSettingsViewModel : GeneralSettingsViewModelT<GeneralSettingsViewModel, implementation::GeneralSettingsViewModel>
    {
    };
}
