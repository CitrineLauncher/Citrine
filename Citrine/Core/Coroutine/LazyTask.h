#pragma once

#include "TaskPromise.h"

namespace Citrine {

	template<typename T = void>
	class [[nodiscard]] LazyTask {
	public:

		using promise_type = TaskPromise<T>;

		constexpr LazyTask() noexcept = default;

		constexpr LazyTask(std::nullptr_t) noexcept : handle(nullptr) {}

		LazyTask(std::coroutine_handle<promise_type> handle) noexcept
			
			: handle(handle)
		{}

		LazyTask(LazyTask const&) = delete;
		auto operator=(LazyTask const&) = delete;

		LazyTask(LazyTask&& other) noexcept

			: handle(std::exchange(other.handle, nullptr))
		{}

		auto operator=(LazyTask&& other) noexcept -> LazyTask& {

			LazyTask{ std::move(other) }.swap(*this);
			return *this;
		}

		auto operator=(std::nullptr_t) noexcept -> LazyTask& {

			LazyTask{ nullptr }.swap(*this);
			return *this;
		}

		explicit operator bool() const noexcept {

			return bool{ handle };
		}

		auto Cancel() -> void {

			handle.promise().Cancel();
		}

		auto IsCancelled() noexcept -> bool {

			return handle.promise().IsCancelled();
		}

		template<typename Self>
		auto ResumeAgile(this Self&& self) -> decltype(auto) {

			self.handle.promise().ResumeAgile();
			return std::forward_like<Self>(self);
		}

		class Awaitable : public winrt::cancellable_awaiter<Awaitable> {
		public:

			Awaitable(std::coroutine_handle<promise_type> handle) noexcept : handle(handle) {}

			auto enable_cancellation(winrt::cancellable_promise* promise) -> void {

				promise->set_canceller([](void* parameter) static {

					std::coroutine_handle<promise_type>::from_address(parameter).promise().Cancel();
				}, handle.address());
			}

			auto await_ready() const noexcept -> bool {

				return handle.promise().AwaitReady();
			}

			template<typename Promise>
			auto await_suspend(std::coroutine_handle<Promise> continuation) noexcept -> bool {

				handle.promise().Start();
				if (handle.promise().AwaitReady())
					return false;

				this->set_cancellable_promise_from_handle(continuation);
				return handle.promise().AwaitSuspend(continuation);
			}

			auto await_resume() -> decltype(auto) {

				return handle.promise().GetResult();
			}

		private:

			std::coroutine_handle<promise_type> handle;
		};

		auto operator co_await() && -> Awaitable {

			return { handle };
		}

		auto swap(LazyTask& other) noexcept -> void {

			std::swap(handle, other.handle);
		}

		~LazyTask() {

			Abandon();
		}

	private:

		auto Abandon() noexcept -> void {

			if (handle)
				handle.promise().Abandon();
		}

		std::coroutine_handle<promise_type> handle{ nullptr };
	};
}