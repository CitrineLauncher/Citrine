// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <safetyhook.hpp>

#include "Core/Util/TrivialArray.h"
#include "Core/Util/Ascii.h"
#include "Minecraft/Bedrock/GameConfig.h"

#include <string_view>
#include <filesystem>
#include <atomic>

#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

using namespace Citrine;
using namespace Minecraft::Bedrock;

namespace {

    using ExceptionHandlerT = auto(*)(::PEXCEPTION_RECORD exceptionRecord, ::PVOID establisherFrame, ::PCONTEXT contextRecord, ::PDISPATCHER_CONTEXT dispatcherContext) -> ::EXCEPTION_DISPOSITION;
    auto cxxFrameHandler4 = ExceptionHandlerT{};
    auto createWindowExW = SafetyHookInline{};

    struct BedrockWinMain {

        void* VTable;
        void* Platform;
        void* Platform_RefCount;
        void* WndProcHandler;
    };

    auto modsDirectory = std::filesystem::path{};
    auto window = ::HWND{};
    auto bedrockWinMain = static_cast<BedrockWinMain const*>(nullptr);
    auto bedrockHasInitialized = static_cast<std::atomic<bool> const*>(nullptr);

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

    auto LoadDlls() -> void try {

        while (!bedrockHasInitialized->load(std::memory_order::relaxed))
            std::this_thread::yield();

        std::atomic_thread_fence(std::memory_order::acquire);

        for (auto const& entry : std::filesystem::directory_iterator{ modsDirectory }) {

            if (!ShouldIncludeDirectoryEntry(entry))
                continue;

            auto const& path = entry.path();
            ::LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        }
    } catch (std::filesystem::filesystem_error const&) {}

    auto $CreateWindowEx(
        ::DWORD dwExStyle,
        ::LPCWSTR lpClassName,
        ::PCWSTR lpWindowName,
        ::DWORD dwStyle,
        int X,
        int Y,
        int nWidth,
        int nHeight,
        ::HWND hWndParent,
        ::HMENU hMenu,
        ::HINSTANCE hInstance,
        ::LPVOID lpParam)
    {
        auto hwnd = createWindowExW.call<::HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        if (IS_INTRESOURCE(lpClassName) || std::wstring_view{ lpClassName } != L"Bedrock") {

            return hwnd;
        }
        static_cast<void>(createWindowExW.disable());

        if (!lpParam)
            return hwnd;

        bedrockWinMain = reinterpret_cast<BedrockWinMain const*>(lpParam);

        constexpr auto subclassProcId = 0x2b5b462b;
        constexpr auto subclassProc = [](
            ::HWND hWnd,
            ::UINT uMsg,
            ::WPARAM wParam,
            ::LPARAM lParam,
            ::UINT_PTR uIdSubclass,
            ::DWORD_PTR dwRefData
        ) static -> ::LRESULT {

            auto result = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
            if (!bedrockWinMain->Platform)
                return result;

            struct AddressPair {

                std::uintptr_t First;
                std::uintptr_t Second;
            };

            auto platformMemory = reinterpret_cast<std::uint8_t const*>(bedrockWinMain->Platform);
            auto [firstAddress, secondAddress] = *reinterpret_cast<AddressPair const*>(&platformMemory[0x20]);

            auto hasInitializedOffset = (firstAddress != 0 && secondAddress != 0 && firstAddress == secondAddress + 16) ? 0x18 : 0x81;
            bedrockHasInitialized = reinterpret_cast<std::atomic<bool> const*>(&platformMemory[hasInitializedOffset]);

            std::thread{ LoadDlls }.detach();

            ::RemoveWindowSubclass(hWnd, reinterpret_cast<::SUBCLASSPROC>(dwRefData), uIdSubclass);
            return result;
        };

        ::SetWindowSubclass(hwnd, subclassProc, subclassProcId, reinterpret_cast<::DWORD_PTR>(+subclassProc));
        return hwnd;
    }

    auto InitializeModLoader(std::filesystem::path modsDirectory) -> void try {

        auto loaderRequired = false;
        for (auto const& entry : std::filesystem::directory_iterator{ modsDirectory }) {

            if (!ShouldIncludeDirectoryEntry(entry))
                continue;

            loaderRequired = true;
            break;
        }

        if (!loaderRequired)
            return;

        ::modsDirectory = std::move(modsDirectory);
        createWindowExW = safetyhook::create_inline(
            reinterpret_cast<void*>(&CreateWindowExW),
            reinterpret_cast<void*>(&$CreateWindowEx)
        );
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

            auto buffer = TrivialArray<wchar_t, MAX_PATH>{};
            auto systemDirectory = std::filesystem::path{ buffer.data(), buffer.data() + ::GetSystemDirectoryW(buffer.data(), buffer.size()) };
            
            if (systemDirectory.empty())
                return FALSE;

            module = ::LoadLibraryExW((systemDirectory / L"vcruntime140_1.dll").c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }

        if (!module)
            return FALSE;

        cxxFrameHandler4 = reinterpret_cast<ExceptionHandlerT>(GetProcAddress(module, "__CxxFrameHandler4"));
        if (!cxxFrameHandler4)
            return FALSE;

        if (modLoaderEnabled)
            InitializeModLoader(currentLocation / L"Mods");
    }
    return TRUE;
}
