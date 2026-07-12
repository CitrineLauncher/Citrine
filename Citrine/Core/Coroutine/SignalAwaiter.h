#pragma once

#include "Promise.h"

#include <atomic>
#include <chrono>
#include <coroutine>
#include <memory>

namespace Citrine {

	class SignalAwaiter : public CancellableAwaiterBase {
	public:

		using TimeoutDuration = std::chrono::file_clock::duration;

		explicit SignalAwaiter(void* handle, TimeoutDuration timeout) noexcept

			: handle(handle)
			, timeout(timeout)
		{}

		auto enable_cancellation(auto* promise) -> decltype(auto) {

			return promise->set_canceller([](void* parameter) static {

				auto self = static_cast<SignalAwaiter*>(parameter);
				if (self->state.exchange(State::Cancelled, std::memory_order::acquire) == State::Waiting) {

					if (self->CancelInternal())
						self->FireImmediately();
				}
			}, this);
		}

		auto await_ready() const noexcept -> bool;

		template<typename Promise>
		auto await_suspend(std::coroutine_handle<Promise> continuation) -> bool {

			waitHandle.reset(CreateThreadPoolWait());
			SetCanceller(continuation);

			if (state.load(std::memory_order::relaxed) == State::Cancelled)
				return false;

			suspending.store(true, std::memory_order::relaxed);
			continuationHandle = continuation;
			SuspendInternal();

			auto expected = State::Idle;
			if (!state.compare_exchange_strong(expected, State::Waiting, std::memory_order::release)) {

				if (CancelInternal())
					return false;
			}
			return suspending.exchange(false, std::memory_order::acquire);
		}

		auto await_resume() -> bool {

			if (state.exchange(State::Idle, std::memory_order::relaxed) == State::Cancelled)
				throw winrt::hresult_canceled{};

			return signalled;
		}

	private:

		auto CreateThreadPoolWait() -> void*;
		auto SuspendInternal() -> void;
		auto CancelInternal() -> bool;
		auto FireImmediately() -> void;

		static auto Callback(void*, void* parameter, void*, std::uint32_t result) -> void {

			auto self = static_cast<SignalAwaiter*>(parameter);
			self->signalled = (result == 0);

			if (self->suspending.exchange(false, std::memory_order::release))
				return;

			self->continuationHandle();
		}

		enum struct State {

			Idle,
			Waiting,
			Cancelled
		};

		struct WaitDeleter {

			static auto operator()(void* handle) noexcept -> void;
		};

		std::atomic<State> state{ State::Idle };
		std::atomic<bool> suspending{ false };
		bool signalled{ false };
		std::unique_ptr<void, WaitDeleter> waitHandle{ nullptr };
		std::coroutine_handle<> continuationHandle{ nullptr };
		void* handle{ nullptr };
		TimeoutDuration timeout{};
	};

	inline auto ResumeOnSignal(void* handle, SignalAwaiter::TimeoutDuration timeout = {}) -> SignalAwaiter {

		return SignalAwaiter{ handle, timeout };
	}
}
