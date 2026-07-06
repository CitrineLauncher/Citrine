#pragma once

#include "IntToBoolConverter.g.h"

namespace winrt::Citrine::implementation
{
    struct IntToBoolConverter : IntToBoolConverterT<IntToBoolConverter>
    {
        IntToBoolConverter() = default;

        auto Convert(
            winrt::Windows::Foundation::IInspectable const& value,
            winrt::Windows::UI::Xaml::Interop::TypeName const&,
            winrt::Windows::Foundation::IInspectable const& parameter,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;

        auto ConvertBack(
            winrt::Windows::Foundation::IInspectable const& value,
            winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
            winrt::Windows::Foundation::IInspectable const& parameter,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct IntToBoolConverter : IntToBoolConverterT<IntToBoolConverter, implementation::IntToBoolConverter>
    {
    };
}
