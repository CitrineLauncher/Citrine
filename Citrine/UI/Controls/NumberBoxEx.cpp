#include "pch.h"
#include "NumberBoxEx.h"
#if __has_include("NumberBoxEx.g.cpp")
#include "NumberBoxEx.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
	NumberBoxEx::NumberBoxEx() {

		EnsureProperties();
	}

	auto NumberBoxEx::ClearButtonVisibility() -> winrt::Visibility {

		return winrt::unbox_value<winrt::Visibility>(GetValue(clearButtonVisibilityProperty));
	}

	auto NumberBoxEx::ClearButtonVisibility(winrt::Visibility value) -> void {

		SetValue(clearButtonVisibilityProperty, winrt::box_value(value));
	}

	auto NumberBoxEx::ClearButtonVisibilityProperty() -> winrt::DependencyProperty {

		return clearButtonVisibilityProperty;
	}

	auto NumberBoxEx::SuffixText() -> winrt::hstring {

		return winrt::unbox_value<winrt::hstring>(GetValue(suffixTextProperty));
	}

	auto NumberBoxEx::SuffixText(winrt::hstring const& value) -> void {

		SetValue(suffixTextProperty, winrt::box_value(value));
	}

	auto NumberBoxEx::SuffixTextProperty() -> winrt::DependencyProperty {

		return suffixTextProperty;
	}

	auto NumberBoxEx::EnsureProperties() -> void {

		if (!clearButtonVisibilityProperty) {

			clearButtonVisibilityProperty = winrt::DependencyProperty::Register(
				L"ClearButtonVisibility",
				winrt::xaml_typename<winrt::Visibility>(),
				winrt::xaml_typename<class_type>(),
				winrt::PropertyMetadata{ winrt::box_value(winrt::Visibility::Visible) }
			);
		}

		if (!suffixTextProperty) {

			suffixTextProperty = winrt::DependencyProperty::Register(
				L"SuffixText",
				winrt::xaml_typename<winrt::hstring>(),
				winrt::xaml_typename<class_type>(),
				winrt::PropertyMetadata{ winrt::box_value(winrt::hstring{}) }
			);
		}
	}

	winrt::Microsoft::UI::Xaml::DependencyProperty NumberBoxEx::clearButtonVisibilityProperty{ nullptr };
	winrt::Microsoft::UI::Xaml::DependencyProperty NumberBoxEx::suffixTextProperty{ nullptr };
}
