#pragma once

#include <winrt/Microsoft.UI.Xaml.Interop.h>

namespace Citrine {

	class NotifyCollectionChangedBase {
	public:

		using NotifyCollectionChangedEventHandler = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventHandler;
		using NotifyCollectionChangedAction = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;
		using NotifyCollectionChangedEventArgs = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs;

		NotifyCollectionChangedBase() = default;

		auto CollectionChanged(NotifyCollectionChangedEventHandler const& handler) -> winrt::event_token {

			return collectionChangedEvent.add(handler);
		}

		auto CollectionChanged(winrt::event_token token) -> void {

			collectionChangedEvent.remove(token);
		}

	protected:

		auto OnCollectionChanged(this auto& self, NotifyCollectionChangedEventArgs const& args) -> void {

			self.collectionChangedEvent(self, args);
		}

		auto OnCollectionChanged(this auto& self, NotifyCollectionChangedAction action, winrt::Microsoft::UI::Xaml::Interop::IBindableVector const& newItems, winrt::Microsoft::UI::Xaml::Interop::IBindableVector const& oldItems, std::int32_t newIndex, std::int32_t oldIndex) -> void {

			self.collectionChangedEvent(self, NotifyCollectionChangedEventArgs{ action, newItems, oldItems, newIndex, oldIndex });
		}

	private:

		winrt::event<NotifyCollectionChangedEventHandler> collectionChangedEvent;
	};
}