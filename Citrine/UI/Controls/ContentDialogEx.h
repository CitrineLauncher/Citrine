#pragma once

#include "ContentDialogEx.g.h"

#include "Core/Util/Event.h"

namespace winrt::Citrine::implementation
{
    struct ContentDialogEx : ContentDialogExT<ContentDialogEx>
    {
        ContentDialogEx();
        
    private:

        ::Citrine::EventRevoker requestedThemeChangedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct ContentDialogEx : ContentDialogExT<ContentDialogEx, implementation::ContentDialogEx>
    {
    };
}
