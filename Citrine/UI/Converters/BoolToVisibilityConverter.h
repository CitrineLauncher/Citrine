#pragma once

#include "BoolToVisibilityConverter.g.h"

namespace winrt::Citrine::implementation
{
    struct BoolToVisibilityConverter : BoolToVisibilityConverterT<BoolToVisibilityConverter>
    {
        BoolToVisibilityConverter() = default;

        auto Convert(
            winrt::Windows::Foundation::IInspectable value,
            winrt::Windows::UI::Xaml::Interop::TypeName const&,
            winrt::Windows::Foundation::IInspectable const& parameter,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;

        auto ConvertBack(
            winrt::Windows::Foundation::IInspectable,
            winrt::Windows::UI::Xaml::Interop::TypeName const&,
            winrt::Windows::Foundation::IInspectable const&,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct BoolToVisibilityConverter : BoolToVisibilityConverterT<BoolToVisibilityConverter, implementation::BoolToVisibilityConverter>
    {
    };
}
