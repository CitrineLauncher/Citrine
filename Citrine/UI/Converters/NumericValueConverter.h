#pragma once

#include "NumericValueConverter.g.h"

namespace winrt::Citrine::implementation
{
    struct NumericValueConverter : NumericValueConverterT<NumericValueConverter>
    {
        NumericValueConverter() = default;

        auto Convert(
            winrt::Windows::Foundation::IInspectable const& value,
            winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
            winrt::Windows::Foundation::IInspectable const&,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;

        auto ConvertBack(
            winrt::Windows::Foundation::IInspectable const& value,
            winrt::Windows::UI::Xaml::Interop::TypeName const& targetType,
            winrt::Windows::Foundation::IInspectable const&,
            winrt::hstring const&
        ) -> winrt::Windows::Foundation::IInspectable;

    private:

        template<typename To, typename From>
        static auto NumericValueCast(From value) noexcept -> To;

        auto ConvertInternal(
            winrt::Windows::Foundation::IInspectable const& value,
            winrt::Windows::UI::Xaml::Interop::TypeName const& targetType
        ) -> winrt::Windows::Foundation::IInspectable;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct NumericValueConverter : NumericValueConverterT<NumericValueConverter, implementation::NumericValueConverter>
    {
    };
}
