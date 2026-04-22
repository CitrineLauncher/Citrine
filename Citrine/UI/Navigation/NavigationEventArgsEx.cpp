#include "pch.h"
#include "NavigationEventArgsEx.h"
#if __has_include("NavigationEventArgsEx.g.cpp")
#include "NavigationEventArgsEx.g.cpp"
#endif

#include "Models/ParameterPair.h"

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Microsoft::UI::Xaml::Navigation;
}

namespace winrt::Citrine::implementation
{
	NavigationEventArgsEx::NavigationEventArgsEx(winrt::NavigationEventArgs const& args) {

		auto param = args.Parameter();
		if (context = param.try_as<Citrine::NavigationContext>())
			return;

		if (auto pair = param.try_as<Citrine::ParameterPair>()) {

			auto pairImpl = winrt::get_self<implementation::ParameterPair>(pair);
			context = pairImpl->first.try_as<Citrine::NavigationContext>();
			parameter = pairImpl->second;
		}
	}

	auto NavigationEventArgsEx::Context() const -> Citrine::NavigationContext {

		return context;
	}

	auto NavigationEventArgsEx::Parameter() const -> winrt::Windows::Foundation::IInspectable {

		return parameter;
	}
}
