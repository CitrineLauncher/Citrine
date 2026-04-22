#include "pch.h"
#include "TextBoxEx.h"
#if __has_include("TextBoxEx.g.cpp")
#include "TextBoxEx.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml;
}

namespace winrt::Citrine::implementation
{
	TextBoxEx::TextBoxEx() {

		EnsureProperties();
	}

	auto TextBoxEx::ClearButtonVisibility() -> winrt::Visibility {

		return winrt::unbox_value<winrt::Visibility>(GetValue(clearButtonVisibilityProperty));
	}

	auto TextBoxEx::ClearButtonVisibility(winrt::Visibility value) -> void {

		SetValue(clearButtonVisibilityProperty, winrt::box_value(value));
	}

	auto TextBoxEx::ClearButtonVisibilityProperty() -> winrt::DependencyProperty {

		return clearButtonVisibilityProperty;
	}

	auto TextBoxEx::PrefixText() -> winrt::hstring {

		return winrt::unbox_value<winrt::hstring>(GetValue(prefixTextProperty));
	}

	auto TextBoxEx::PrefixText(winrt::hstring const& value) -> void {

		SetValue(prefixTextProperty, winrt::box_value(value));
	}

	auto TextBoxEx::PrefixTextProperty() -> winrt::DependencyProperty {

		return prefixTextProperty;
	}

	auto TextBoxEx::SuffixText() -> winrt::hstring {

		return winrt::unbox_value<winrt::hstring>(GetValue(suffixTextProperty));
	}

	auto TextBoxEx::SuffixText(winrt::hstring const& value) -> void {

		SetValue(suffixTextProperty, winrt::box_value(value));
	}

	auto TextBoxEx::SuffixTextProperty() -> winrt::DependencyProperty {

		return suffixTextProperty;
	}

	auto TextBoxEx::EnsureProperties() -> void {

		if (!clearButtonVisibilityProperty) {

			clearButtonVisibilityProperty = winrt::DependencyProperty::Register(
				L"ClearButtonVisibility",
				winrt::xaml_typename<winrt::Visibility>(),
				winrt::xaml_typename<class_type>(),
				winrt::PropertyMetadata{ winrt::box_value(winrt::Visibility::Visible) }
			);
		}

		if (!prefixTextProperty) {

			prefixTextProperty = winrt::DependencyProperty::Register(
				L"PrefixText",
				winrt::xaml_typename<winrt::hstring>(),
				winrt::xaml_typename<class_type>(),
				winrt::PropertyMetadata{ winrt::box_value(winrt::hstring{}) }
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

	winrt::Microsoft::UI::Xaml::DependencyProperty TextBoxEx::clearButtonVisibilityProperty{ nullptr };
	winrt::Microsoft::UI::Xaml::DependencyProperty TextBoxEx::prefixTextProperty{ nullptr };
	winrt::Microsoft::UI::Xaml::DependencyProperty TextBoxEx::suffixTextProperty{ nullptr };
}
