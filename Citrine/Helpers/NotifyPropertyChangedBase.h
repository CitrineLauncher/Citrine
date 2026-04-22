#pragma once

#include <winrt/Microsoft.UI.Xaml.Data.h>

namespace Citrine {

	class NotifyPropertyChangedBase {
	public:

		using PropertyChangedEventHandler = winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler;
		using PropertyChangedEventArgs = winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs;

		NotifyPropertyChangedBase() = default;

		auto PropertyChanged(PropertyChangedEventHandler const& handler) -> winrt::event_token {

			return propertyChangedEvent.add(handler);
		}

		auto PropertyChanged(winrt::event_token token) -> void {

			propertyChangedEvent.remove(token);
		}

	protected:

		auto OnPropertyChanged(this auto& self, PropertyChangedEventArgs const& args) -> void {

			self.propertyChangedEvent(self, args);
		}

		auto OnPropertyChanged(this auto& self, winrt::hstring const& propertyName) -> void {

			self.propertyChangedEvent(self, PropertyChangedEventArgs{ propertyName });
		}

	private:

		winrt::event<PropertyChangedEventHandler> propertyChangedEvent;
	};
}