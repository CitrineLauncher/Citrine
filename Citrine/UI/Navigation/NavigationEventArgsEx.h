#pragma once

#include "NavigationEventArgsEx.g.h"

namespace winrt::Citrine::implementation
{
    struct NavigationEventArgsEx : NavigationEventArgsExT<NavigationEventArgsEx>
    {
        NavigationEventArgsEx(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& args);

        auto Context() const -> Citrine::NavigationContext;
        auto Parameter() const -> winrt::Windows::Foundation::IInspectable;

        Citrine::NavigationContext context{ nullptr };
        winrt::Windows::Foundation::IInspectable parameter{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct NavigationEventArgsEx : NavigationEventArgsExT<NavigationEventArgsEx, implementation::NavigationEventArgsEx>
    {
    };
}
