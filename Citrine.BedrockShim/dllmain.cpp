// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "Core/Util/TrivialArray.h"
#include "Core/Util/Ascii.h"
#include "Minecraft/Bedrock/GameConfig.h"

#include <string_view>
#include <filesystem>
#include <atomic>
#include <thread>
#include <chrono>

#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

using namespace Citrine;
using namespace Minecraft::Bedrock;

using namespace std::chrono_literals;

namespace {

    using ExceptionHandlerT = auto(*)(::PEXCEPTION_RECORD exceptionRecord, ::PVOID establisherFrame, ::PCONTEXT contextRecord, ::PDISPATCHER_CONTEXT dispatcherContext) -> ::EXCEPTION_DISPOSITION;
    auto cxxFrameHandler4 = ExceptionHandlerT{};

    struct ModEntry {

        std::filesystem::path Path;
        ::HMODULE Module{ nullptr };
    };

    enum struct ModLoaderStatus {

        Idle,
        Loading,
        LoadingCompleted,
        Unloading,
        UnloadingCompleted
    };

    auto modEntries = std::vector<ModEntry>{};
    auto modLoaderStatus = std::atomic<ModLoaderStatus>{};
    auto modsLoadDelay = std::chrono::milliseconds{};
    auto getMessageHook = ::HHOOK{};

    auto IsHiddenDirectoryEntry(std::filesystem::directory_entry const& entry) -> bool {

        auto const& path = entry.path();
        auto attributes = ::GetFileAttributesW(path.c_str());

        return attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }

    auto ShouldIncludeDirectoryEntry(std::filesystem::directory_entry const& entry) -> bool {

        if (!entry.is_regular_file() || IsHiddenDirectoryEntry(entry))
            return false;

        auto const& path = entry.path();
        auto filename = path.filename();
        auto extension = path.extension();

        if (!Ascii::CaseInsensitiveEquals(extension.native(), L".dll"))
            return false;

        if (Ascii::CaseInsensitiveEquals(filename.native(), L"vcruntime140_1.dll"))
            return false;

        return true;
    }

    auto LoadDlls() -> void {

        for (auto& entry : ::modEntries) {

            entry.Module = ::LoadLibraryExW(entry.Path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        }
    }

    auto UnloadDlls() -> void {

        for (auto const& entry : ::modEntries | std::views::reverse) {

            if (entry.Module)
                ::FreeLibrary(entry.Module);
        }
    }

    auto InitializeModLoader(std::filesystem::path const& modsDirectory, std::chrono::milliseconds loadDelay) -> void try {

        for (auto const& entry : std::filesystem::directory_iterator{ modsDirectory }) {

            if (!ShouldIncludeDirectoryEntry(entry))
                continue;

            ::modEntries.emplace_back(entry);
            break;
        }

        if (::modEntries.empty())
            return;

        ::modsLoadDelay = std::clamp(loadDelay, 0ms, 15000ms);

        constexpr auto hookProc = [](int code, ::WPARAM wParam, ::LPARAM lParam) static -> ::LRESULT {

            auto message = reinterpret_cast<::MSG const*>(lParam)->message;
            if (message >= WM_APP && modLoaderStatus.load(std::memory_order::relaxed) == ModLoaderStatus::Idle) {

                modLoaderStatus.store(ModLoaderStatus::Loading, std::memory_order::relaxed);
                std::thread{ [] static {

                    std::this_thread::sleep_for(::modsLoadDelay);
                    LoadDlls();

                    modLoaderStatus.store(ModLoaderStatus::LoadingCompleted, std::memory_order::release);
                } }
                .detach();
            }
            else if (message == WM_QUIT) {

                ::TerminateProcess(::GetCurrentProcess(), 0);
            }
            return ::CallNextHookEx(nullptr, code, wParam, lParam);
        };

        ::getMessageHook = ::SetWindowsHookExW(WH_GETMESSAGE, hookProc, nullptr, GetCurrentThreadId());
    }
    catch (std::filesystem::filesystem_error const&) {}
}

#pragma comment(linker, "/export:__CxxFrameHandler4=$CxxFrameHandler4")
extern "C" auto $CxxFrameHandler4(::PEXCEPTION_RECORD exceptionRecord, ::PVOID establisherFrame, ::PCONTEXT contextRecord, ::PDISPATCHER_CONTEXT dispatcherContext) -> ::EXCEPTION_DISPOSITION {

    return cxxFrameHandler4(exceptionRecord, establisherFrame, contextRecord, dispatcherContext);
}

auto APIENTRY DllMain(HMODULE currentModule, DWORD reason, LPVOID reserved) -> BOOL
{
    if (reason == DLL_PROCESS_ATTACH) {

        ::DisableThreadLibraryCalls(currentModule);

        if (auto aumid = std::wstring{}; wil::TryGetEnvironmentVariableW(L"CitrineLauncher_GameInstance_Aumid", aumid) == S_OK) {

            ::SetCurrentProcessExplicitAppUserModelID(aumid.c_str());
        }

        auto currentLocation = [currentModule] {

            auto path = std::filesystem::path{ wil::GetModuleFileNameW<std::wstring>(currentModule) };
            path.remove_filename();
            return path;
        }();

        auto gameConfig = GameConfig::Load(currentLocation / "Citrine.GameConfig.json");
        auto modLoaderEnabled = gameConfig && gameConfig->ModLoaderEnabled;

        auto module = ::HMODULE{};
        if (modLoaderEnabled) {

            module = ::LoadLibraryExW((currentLocation / LR"(Mods\vcruntime140_1.dll)").c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        }

        if (!module) {

            auto vcRuntimeHandle = ::GetModuleHandleW(L"vcruntime140.dll");
            if (!vcRuntimeHandle)
                return FALSE;

            auto vcRuntimeDirectory = [vcRuntimeHandle] {

                auto path = std::filesystem::path{ wil::GetModuleFileNameW<std::wstring>(vcRuntimeHandle) };
                path.remove_filename();
                return path;
            }();

            if (auto ec = std::error_code{}; std::filesystem::equivalent(vcRuntimeDirectory, currentLocation))
                vcRuntimeDirectory = wil::GetSystemDirectoryW<std::wstring>();

            module = ::LoadLibraryExW((vcRuntimeDirectory / L"vcruntime140_1.dll").c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        }

        if (!module)
            return FALSE;

        cxxFrameHandler4 = reinterpret_cast<ExceptionHandlerT>(GetProcAddress(module, "__CxxFrameHandler4"));
        if (!cxxFrameHandler4)
            return FALSE;

        if (modLoaderEnabled)
            InitializeModLoader(currentLocation / L"Mods", gameConfig->ModsLoadDelay);
    }
    return TRUE;
}