#include "pch.h"
#include "OneTimeNavigationTransitionInfo.h"
#if __has_include("OneTimeNavigationTransitionInfo.g.cpp")
#include "OneTimeNavigationTransitionInfo.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation::Collections;
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Media::Animation;
}

namespace winrt::Citrine::implementation
{
    OneTimeNavigationTransitionInfo::OneTimeNavigationTransitionInfo(winrt::NavigationTransitionInfo const& transitionInfo, Citrine::NavigationDirection direction)

        : transitionInfo(transitionInfo.try_as<winrt::INavigationTransitionInfoPrivate>())
        , direction(direction)
    {}

    auto OneTimeNavigationTransitionInfo::CreateStoryboards(winrt::UIElement const& element, winrt::NavigationTrigger trigger, winrt::IVector<winrt::Storyboard> const& storyboards) -> void {

        if (!transitionInfo)
            return;

        trigger = winrt::NavigationTrigger{ std::to_underlying(trigger) & (1 << 0) };
        if (direction == NavigationDirection::Backward)
            trigger = winrt::NavigationTrigger{ std::to_underlying(trigger) | (1 << 1) };

        transitionInfo.CreateStoryboards(element, trigger, storyboards);

        if ((std::to_underlying(trigger) & (1 << 0)) != 0)
            transitionInfo = nullptr;
    }
}
