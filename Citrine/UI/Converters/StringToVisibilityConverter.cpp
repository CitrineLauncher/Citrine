#include "pch.h"
#include "StringToVisibilityConverter.h"
#if __has_include("StringToVisibilityConverter.g.cpp")
#include "StringToVisibilityConverter.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::UI::Xaml::Interop;
	using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
    auto StringToVisibilityConverter::Convert(winrt::IInspectable value, winrt::TypeName const&, winrt::IInspectable const& parameter, winrt::hstring const&) -> winrt::IInspectable {

        auto visible = value && winrt::unbox_value<winrt::hstring>(value).size() > 0;
        auto inverted = parameter && winrt::unbox_value<winrt::hstring>(parameter) == L"Inverted";

        return winrt::box_value(visible != inverted ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }

    auto StringToVisibilityConverter::ConvertBack(winrt::IInspectable, winrt::TypeName const&, winrt::IInspectable const&, winrt::hstring const&) -> winrt::IInspectable {

        throw winrt::hresult_not_implemented{};
    }
}
