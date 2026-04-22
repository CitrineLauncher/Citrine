#include "pch.h"
#include "App.xaml.h"

#include "Citrine.h"

#include "ApplicationData.h"

#include "Core/Logging/Logger.h"
#include "Locale/Localizer.h"
#include "UI/Views/Windows/MainWindow.xaml.h"
#include "Services/MinecraftBedrockGameManager.h"

#include <WinUser.h>
#include <processthreadsapi.h>

#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

namespace winrt {

    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Dispatching;
}

using namespace ::Citrine;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Citrine::implementation
{
    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App(::Citrine::ApplicationMutex&& mutex)
    
        : mutex(std::move(mutex))
        , executablePath(wil::GetModuleFileNameW<std::wstring>())
    {

        current = this;

        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    auto App::Current() noexcept -> App& {

        return *current;
    }

    auto App::InitializeComponent() -> void {

        AppT::InitializeComponent();
        dispatcher = winrt::DispatcherQueue::GetForCurrentThread();

        ApplicationData::Initialize();
        Localizer::Initialize(ApplicationData::LocalSettings().Language());
        Logger::Initialize(ApplicationData::LocalLogDirectory() / L"CitrineLauncher.log");

        Logger::Info("Citrine {}", CITRINE_PRODUCTVERSION);
        MinecraftBedrockGameManager::InitializeAsync();
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    auto App::OnLaunched(LaunchActivatedEventArgs const&) -> void {

        DispatcherShutdownMode(winrt::DispatcherShutdownMode::OnExplicitShutdown);

        mainWindow = winrt::make_self<implementation::MainWindow>();
        mainWindow->Activate();
    }

    auto App::Version() const noexcept -> ::Citrine::SemanticVersion {

        return { CITRINE_VERSION_MAJOR, CITRINE_VERSION_MINOR, CITRINE_VERSION_PATCH };
    }

    auto App::ExecutablePath() const noexcept -> std::filesystem::path const& {

        return executablePath;
    }

    auto App::Cleanup() -> void {

        MinecraftBedrockGameManager::Shutdown();

        auto& settings = ApplicationData::LocalSettings();
        settings.Save();
        settings.Close();
        Logger::Shutdown();
    }

    auto App::CleanupAndExitAsync() -> void {

        if (std::exchange(exitRequested, true))
            return;

        dispatcher.TryEnqueue([self = get_strong()] {

            self->Cleanup();
            self->mutex.Close();

            if (self->restartRequested) {

                auto startupInfo = ::STARTUPINFO{ sizeof(::STARTUPINFO) };
                auto processInfo = wil::unique_process_information{};
                ::CreateProcessW(self->executablePath.c_str(), nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInfo);
            }
            self->Exit();
        });
    }

    auto App::Restart() -> void {

        if (std::exchange(restartRequested, true))
            return;

        if (mainWindow)
            ::SendMessageW(mainWindow->NativeHandle(), WM_CLOSE, 0, 0);
    }

    constinit App* App::current{ nullptr };
}
