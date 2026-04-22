#include "pch.h"
#include "NavigationContext.h"
#if __has_include("NavigationContext.g.cpp")
#include "NavigationContext.g.cpp"
#endif

#include "OneTimeNavigationTransitionInfo.h"

#include "UI/Controls/FrameEx.h"
#include "Models/ParameterPair.h"

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::UI::Xaml::Interop;
    using namespace Windows::UI::Xaml::Input;
    using namespace Microsoft::UI::Xaml::Media::Animation;
}

namespace {

    auto WrapTransitionInfo(winrt::NavigationTransitionInfo const& transitionInfo, winrt::Citrine::NavigationDirection direction) -> auto {

        return winrt::Citrine::OneTimeNavigationTransitionInfo{ transitionInfo, direction };
    }
}

namespace winrt::Citrine::implementation
{
    NavigationContext::NavigationContext(Citrine::FrameEx const& frame, winrt::TypeName sourcePageType)
    
        : frame(frame)
        , currentSourcePageType(std::move(sourcePageType))
    {}

    auto NavigationContext::IsActive() const noexcept -> bool {

        auto frameImpl = winrt::get_self<implementation::FrameEx>(frame);
        return frameImpl->activeContext == this;
    }

    auto NavigationContext::MakeActive(winrt::NavigationTransitionInfo transitionInfo) -> bool {

        auto frameImpl = winrt::get_self<implementation::FrameEx>(frame);
        if (frameImpl->activeContext == this)
            return true;

        if (frameImpl->activeContext == nullptr)
            transitionInfo = nullptr;

        if (currentSourcePageType.Name.empty()) {

            frame.Content(nullptr);
        }
        else if (!frame.Navigate(currentSourcePageType, nullptr, WrapTransitionInfo(transitionInfo, NavigationDirection::Forward))) {

            return false;
        }

        frameImpl->activeContext = this;
        return true;
    }

    auto NavigationContext::CurrentSourcePageType() const -> winrt::TypeName {

        return currentSourcePageType;
    }

    auto NavigationContext::Navigate(winrt::TypeName sourcePageType, winrt::IInspectable parameter, winrt::NavigationTransitionInfo transitionInfo) -> bool {

        if (!IsActive())
            return false;

        if (parameter)
            parameter = winrt::make<implementation::ParameterPair>(*this, std::move(parameter));
        else
            parameter = *this;

        if (!frame.Navigate(sourcePageType, parameter, WrapTransitionInfo(transitionInfo, NavigationDirection::Forward)))
            return false;

        backStack.emplace_back(std::move(currentSourcePageType), std::move(transitionInfo));
        currentSourcePageType = std::move(sourcePageType);
        return true;
    }

    auto NavigationContext::CanNavigateBack() const noexcept -> bool {

        return backStack.size() > 0;
    }

    auto NavigationContext::NavigateBack(winrt::IInspectable parameter, winrt::NavigationTransitionInfo const& transitionInfoOverride) -> bool {

        if (!IsActive() || backStack.empty())
            return false;

        if (parameter)
            parameter = winrt::make<implementation::ParameterPair>(*this, std::move(parameter));
        else
            parameter = *this;

        auto& prevPage = backStack.back();
        auto transitionInfo = transitionInfoOverride
            ? transitionInfoOverride
            : prevPage.TransitionInfo;

        if (!frame.Navigate(prevPage.SourcePageType, parameter, WrapTransitionInfo(transitionInfo, NavigationDirection::Backward)))
            return false;

        currentSourcePageType = std::move(prevPage.SourcePageType);
        backStack.pop_back();
        return true;
    }

    NavigationContext::~NavigationContext() noexcept {

        auto frameImpl = winrt::get_self<implementation::FrameEx>(frame);
        if (frameImpl->activeContext == this)
            frameImpl->activeContext = nullptr;
    }
}
