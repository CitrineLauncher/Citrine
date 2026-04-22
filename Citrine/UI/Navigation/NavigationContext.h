#pragma once

#include "NavigationContext.g.h"

#include <vector>

namespace winrt::Citrine::implementation
{
    struct NavigationContext : NavigationContextT<NavigationContext>
    {
        NavigationContext(Citrine::FrameEx const& frame, winrt::Windows::UI::Xaml::Interop::TypeName sourcePageType = {});

        auto IsActive() const noexcept -> bool;
        auto MakeActive(
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo transitionInfo = nullptr
        ) -> bool;

        auto CurrentSourcePageType() const -> winrt::Windows::UI::Xaml::Interop::TypeName;
        auto Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName sourcePageType,
            winrt::Windows::Foundation::IInspectable parameter = nullptr,
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo transitionInfo = nullptr
        ) -> bool;

        auto CanNavigateBack() const noexcept -> bool;
        auto NavigateBack(
            winrt::Windows::Foundation::IInspectable parameter = nullptr,
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo const& transitionInfoOverride = nullptr
        ) -> bool;

        ~NavigationContext() noexcept;

    private:

        struct BackStackEntry {

            winrt::Windows::UI::Xaml::Interop::TypeName SourcePageType{};
            winrt::Microsoft::UI::Xaml::Media::Animation::NavigationTransitionInfo TransitionInfo{ nullptr };
        };

        Citrine::FrameEx frame{ nullptr };
        winrt::Windows::UI::Xaml::Interop::TypeName currentSourcePageType;
        std::vector<BackStackEntry> backStack;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct NavigationContext : NavigationContextT<NavigationContext, implementation::NavigationContext>
    {
    };
}
