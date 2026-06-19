#pragma once

#include <cstdint>
#include <coroutine>
#include <atomic>
#include <winrt/base.h>

namespace Citrine {

	class PromiseBase {
	protected:

		struct StateSentinel {

			operator void* (this StateSentinel self) noexcept {

				return reinterpret_cast<void*>(self.Value);
			}

			std::uintptr_t Value{};
		};
	};

	class CancellablePromiseBase : public PromiseBase, public winrt::cancellable_promise {
	public:

		CancellablePromiseBase() noexcept {

			enable_cancellation_propagation(true);
		}

		CancellablePromiseBase(CancellablePromiseBase const&) = delete;
		auto operator=(CancellablePromiseBase const&) = delete;

		auto set_canceller(canceller_t canceller, void* param) -> bool {

			auto expected = static_cast<void*>(nullptr);

			cancellerParam = param;
			if (!cancellerState.compare_exchange_strong(expected, reinterpret_cast<void*>(canceller), std::memory_order::release, std::memory_order::relaxed)) {

				canceller(param);
				return true;
			}
			return false;
		}

		auto revoke_canceller() -> void {

			auto canceller = cancellerState.load(std::memory_order::relaxed);
			do {

				if (canceller == CancellerState::Cancelling) {

					cancellerState.wait(CancellerState::Cancelling, std::memory_order::acquire);
					break;
				}
				else if (canceller == CancellerState::Cancelled) {

					std::atomic_thread_fence(std::memory_order::acquire);
					break;
				}
			} while (!cancellerState.compare_exchange_strong(canceller, nullptr, std::memory_order::acquire, std::memory_order::relaxed));
		}

		auto Cancel() -> void {

			struct Lock {

				~Lock() noexcept {

					promise.cancellerState.store(CancellerState::Cancelled, std::memory_order::release);
					promise.cancellerState.notify_one();
				}

				CancellablePromiseBase& promise;
			};

			auto canceller = cancellerState.load(std::memory_order::relaxed);
			do {

				if (canceller == CancellerState::Cancelling || canceller == CancellerState::Cancelled)
					return;

			} while (!cancellerState.compare_exchange_strong(canceller, CancellerState::Cancelling, std::memory_order::acquire, std::memory_order::relaxed));

			auto lock = Lock{ *this };
			if (canceller) {

				reinterpret_cast<canceller_t>(canceller)(cancellerParam);
			}

			cancellable_promise::cancel();
		}

	private:

		struct CancellerState {

			static constexpr auto Cancelling = StateSentinel{ 1 };
			static constexpr auto Cancelled = StateSentinel{ 2 };
		};

		std::atomic<void*> cancellerState{ nullptr };
		void* cancellerParam{ nullptr };
	};

	class CancellableAwaiterBase {
	public:

		CancellableAwaiterBase() noexcept = default;

		CancellableAwaiterBase(CancellableAwaiterBase const&) = delete;
		auto operator=(CancellableAwaiterBase const&) = delete;

		~CancellableAwaiterBase() {

			if (promise) {

				promise->revoke_canceller();
			}
			else if (winRTPromise) {

				winRTPromise->revoke_canceller();
			}
		}

	protected:

		template<typename Promise>
		auto SetCanceller(this auto& self, std::coroutine_handle<Promise> canceller) -> bool {

			if constexpr (std::derived_from<Promise, CancellablePromiseBase>) {

				self.promise = &canceller.promise();
				return self.enable_cancellation(self.promise);
			}
			else if constexpr (std::derived_from<Promise, winrt::cancellable_promise>) {

				self.winRTPromise = &canceller.promise();
				self.enable_cancellation(self.winRTPromise);

				return false;
			}
			else {

				return false;
			}
		}

	private:

		CancellablePromiseBase* promise{ nullptr };
		winrt::cancellable_promise* winRTPromise{ nullptr };
	};
}