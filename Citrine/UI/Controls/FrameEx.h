#pragma once

#include "FrameEx.g.h"

namespace winrt::Citrine::implementation
{
    struct NavigationContext;

    struct FrameEx : FrameExT<FrameEx>
    {
        FrameEx() = default;

    private:

        friend implementation::NavigationContext;

        implementation::NavigationContext* activeContext{ nullptr };
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct FrameEx : FrameExT<FrameEx, implementation::FrameEx>
    {
    };
}
