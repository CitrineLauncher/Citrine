#pragma once

#include "App.xaml.g.h"

#include "ApplicationMutex.h"
#include "Core/Util/SemanticVersion.h"
#include "UI/Controls/WindowEx.h"

#include <filesystem>

namespace winrt::Citrine::implementation
{
    struct App : AppT<App>
    {
        static constexpr auto& MutexName = L"CitrineLauncher_69df74b0-437b-4c96-9845-7ece0b35588a";
        static constexpr auto& RedirectEventName = L"CitrineLauncher_RedirectEvent";

        App(::Citrine::ApplicationMutex&& mutex);

        static auto Current() noexcept -> App&;

        auto InitializeComponent() -> void;
        auto OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) -> void;

        auto Version() const noexcept -> ::Citrine::SemanticVersion;
        auto ExecutablePath() const noexcept -> std::filesystem::path const&;

        auto Cleanup() -> void;
        auto CleanupAndExitAsync() -> void;

        auto Restart() -> void;

    private:

        static constinit App* current;

        ::Citrine::ApplicationMutex mutex;
        winrt::com_ptr<implementation::WindowEx> mainWindow{ nullptr };
        winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher{ nullptr };
        std::filesystem::path executablePath;
        bool exitRequested{};
        bool restartRequested{};
    };
}
