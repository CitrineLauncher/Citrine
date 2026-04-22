#pragma once

#include "winrt/Citrine.h"

#include <type_traits>
#include <concepts>

namespace winrt::Citrine::implementation {

	template<bool CanExecuteChangedEnabled = true>
	class RelayCommandBase {
	public:

		using CanExecuteChangedEventHandler = winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable>;

		auto CanExecuteChanged(CanExecuteChangedEventHandler const& handler) -> winrt::event_token {

			if constexpr (CanExecuteChangedEnabled) {

				return canExecuteChangedEvent.add(handler);
			}
			else {

				static_cast<void>(handler);
				return {};
			}
		}

		auto CanExecuteChanged(winrt::event_token token) -> void {

			if constexpr (CanExecuteChangedEnabled) {

				canExecuteChangedEvent.remove(token);
			}
			else {

				static_cast<void>(token);
			}
		}

	protected:

		auto OnCanExecuteChanged(this auto& self) -> void {

			if constexpr (CanExecuteChangedEnabled) {

				self.canExecuteChangedEvent(self, nullptr);
			}
		}

	private:

		struct EmptyEvent{};

		[[no_unique_address, msvc::no_unique_address]]
		std::conditional_t<CanExecuteChangedEnabled, winrt::event<CanExecuteChangedEventHandler>, EmptyEvent> canExecuteChangedEvent;
	};

	template<typename Func, typename Predicate = void>
	class RelayCommand : public winrt::implements<RelayCommand<Func, Predicate>, Citrine::IRelayCommand, winrt::Microsoft::UI::Xaml::Input::ICommand>, public RelayCommandBase<!std::same_as<void, Predicate>> {
	public:

		template<typename F>
		RelayCommand(F&& func) requires (std::same_as<Predicate, void>)

			: func(std::forward<F>(func))
		{}

		template<typename F, typename P>
		RelayCommand(F&& func, P&& canExecute) requires (!std::same_as<Predicate, void>)

			: func(std::forward<F>(func))
			, canExecute(std::forward<P>(canExecute))
		{}

		auto CanExecute(winrt::Windows::Foundation::IInspectable const& parameter) const -> bool {

			if constexpr (std::same_as<Predicate, void>) {

				return true;
			}
			else if constexpr (std::invocable<Predicate, decltype(parameter)>) {

				return canExecute(parameter);
			}
			else {

				return canExecute();
			}
		}

		auto Execute(winrt::Windows::Foundation::IInspectable const& parameter) const -> void {

			if constexpr (std::invocable<Func, decltype(parameter)>) {

				func(parameter);
			}
			else {

				func();
			}
		}

		auto NotifyCanExecuteChanged() -> void {

			this->OnCanExecuteChanged();
		}

	private:

		struct EmptyPredicate {};

		[[no_unique_address, msvc::no_unique_address]]
		Func func;
		[[no_unique_address, msvc::no_unique_address]]
		std::conditional_t<!std::same_as<Predicate, void>, Predicate, EmptyPredicate> canExecute;
	};
	
	template<typename F>
	auto MakeRelayCommand(F&& func) -> Citrine::IRelayCommand {

		using CommandT = RelayCommand<std::decay_t<F>>;
		return winrt::make<CommandT>(std::forward<F>(func));
	}

	template<typename F, typename P>
	auto MakeRelayCommand(F&& func, P&& canExecute) -> Citrine::IRelayCommand {

		using CommandT = RelayCommand<std::decay_t<F>, std::decay_t<P>>;
		return winrt::make<CommandT>(std::forward<F>(func), std::forward<P>(canExecute));
	}
}
