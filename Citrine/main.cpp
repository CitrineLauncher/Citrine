#include "pch.h"

#include "ApplicationMutex.h"
#include "App.xaml.h"

using namespace Citrine;
using winrt::Citrine::implementation::App;

namespace winrt {

    using namespace Microsoft::UI::Xaml;
}

auto __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) -> int {

    winrt::init_apartment(winrt::apartment_type::single_threaded);

    auto mutex = ApplicationMutex{ App::MutexName };
    if (!mutex) {

        ::MessageBoxW(nullptr, L"Creating single instance mutex failed.", L"Citrine", MB_ICONERROR | MB_OK);
        std::quick_exit(0);
    }

    if (!mutex.IsLocked()) {

        auto redirectEventId = ::RegisterWindowMessageW(App::RedirectEventName);
        PostMessageW(HWND_BROADCAST, redirectEventId, 0, 0);
    }
    else {

        winrt::Application::Start([&](auto&&) { winrt::make<App>(std::move(mutex)); });
    }
    std::quick_exit(0);
}