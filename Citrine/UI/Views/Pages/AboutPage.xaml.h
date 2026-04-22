#pragma once

#include "AboutPage.g.h"

#include "UI/ViewModels/AboutViewModel.h"

namespace winrt::Citrine::implementation
{
    struct AboutPage : AboutPageT<AboutPage>
    {
        AboutPage() = default;

        auto ViewModel() const noexcept -> Citrine::AboutViewModel;

    private:

        winrt::com_ptr<implementation::AboutViewModel> viewModel = winrt::make_self<implementation::AboutViewModel>();
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct AboutPage : AboutPageT<AboutPage, implementation::AboutPage>
    {
    };
}
