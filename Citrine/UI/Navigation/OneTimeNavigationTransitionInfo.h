#pragma once

#include "OneTimeNavigationTransitionInfo.g.h"

namespace winrt::Citrine::implementation
{
    struct OneTimeNavigationTransitionInfo : OneTimeNavigationTransitionInfoT<OneTimeNavigationTransitionInfo, winrt::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoPrivate>
    {
        OneTimeNavigationTransitionInfo(
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo const& transitionInfo,
            Citrine::NavigationDirection direction
        );

        auto CreateStoryboards(
            winrt::Microsoft::UI::Xaml::UIElement const& element,
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTrigger trigger,
            winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Media::Animation::Storyboard> const& storyboards
        ) -> void;

    private:

        winrt::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfoPrivate transitionInfo{ nullptr };
        Citrine::NavigationDirection direction{};
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct OneTimeNavigationTransitionInfo : OneTimeNavigationTransitionInfoT<OneTimeNavigationTransitionInfo, implementation::OneTimeNavigationTransitionInfo>
    {
    };
}
