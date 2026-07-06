#include "pch.h"
#include "BoolToVisibilityConverter.h"
#if __has_include("BoolToVisibilityConverter.g.cpp")
#include "BoolToVisibilityConverter.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::UI::Xaml::Interop;
	using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
    auto BoolToVisibilityConverter::Convert(winrt::IInspectable const& value, winrt::TypeName const&, winrt::IInspectable const& parameter, winrt::hstring const&) -> winrt::IInspectable {

        auto visible = value && winrt::unbox_value<bool>(value);
        auto inverted = parameter && winrt::unbox_value<winrt::hstring>(parameter) == L"Inverted";

        return winrt::box_value(visible != inverted ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }

    auto BoolToVisibilityConverter::ConvertBack(winrt::IInspectable const& value, winrt::TypeName const&, winrt::IInspectable const& parameter, winrt::hstring const&) -> winrt::IInspectable {

        auto visible = value && winrt::unbox_value<winrt::Visibility>(value) == winrt::Visibility::Visible;
        auto inverted = parameter && winrt::unbox_value<winrt::hstring>(parameter) == L"Inverted";

        return winrt::box_value(visible != inverted);
    }
}
