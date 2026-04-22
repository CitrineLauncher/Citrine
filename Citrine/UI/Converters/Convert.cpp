#include "pch.h"
#include "Convert.h"
#if __has_include("Convert.g.cpp")
#include "Convert.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation;
    using namespace winrt::Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
    auto Convert::BoolToInverseBool(bool value) noexcept -> bool {

        return !value;
    }

    auto Convert::BoolToVisibility(bool value) noexcept -> winrt::Visibility {

        return value
            ? winrt::Visibility::Visible
            : winrt::Visibility::Collapsed;
    }

    auto Convert::BoolToInverseVisibility(bool value) noexcept -> winrt::Visibility {

        return value
            ? winrt::Visibility::Collapsed
            : winrt::Visibility::Visible;
    }

    auto Convert::NullToBool(winrt::Windows::Foundation::IInspectable const& value) noexcept -> bool {

        return static_cast<bool>(value);
    }

    auto Convert::NullToInverseBool(winrt::Windows::Foundation::IInspectable const& value) noexcept -> bool {

        return !static_cast<bool>(value);
    }

    auto Convert::NullToVisibility(winrt::IInspectable const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility {

        return value
            ? winrt::Visibility::Visible
            : winrt::Visibility::Collapsed;
    }

    auto Convert::NullToInverseVisibility(winrt::IInspectable const& value) noexcept -> winrt::Microsoft::UI::Xaml::Visibility {

        return value
            ? winrt::Visibility::Collapsed
            : winrt::Visibility::Visible;
    }

    auto Convert::StringToBool(winrt::hstring const& value) noexcept -> bool {

        return !value.empty();
    }

    auto Convert::StringToInverseBool(winrt::hstring const& value) noexcept -> bool {

        return value.empty();
    }

    auto Convert::StringToVisibility(winrt::hstring const& value) noexcept -> winrt::Visibility {

        return value.empty()
            ? winrt::Visibility::Collapsed
            : winrt::Visibility::Visible;
    }

    auto Convert::StringToInverseVisibility(winrt::hstring const& value) noexcept -> winrt::Visibility {

        return value.empty()
            ? winrt::Visibility::Visible
            : winrt::Visibility::Collapsed;
    }
}
