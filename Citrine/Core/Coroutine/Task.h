#pragma once

#include "TaskPromise.h"

namespace Citrine {

	template<typename T = void>
	class [[nodiscard]] Task {
	public:

		using promise_type = TaskPromise<T>;

		constexpr Task() noexcept = default;

		constexpr Task(std::nullptr_t) noexcept : handle(nullptr) {}

		Task(std::coroutine_handle<promise_type> handle) noexcept
			
			: handle(handle)
		{
			handle.promise().Start();
		}

		Task(Task const&) = delete;
		auto operator=(Task const&) = delete;

		Task(Task&& other) noexcept

			: handle(std::exchange(other.handle, nullptr))
		{}

		auto operator=(Task&& other) noexcept -> Task& {

			Task{ std::move(other) }.swap(*this);
			return *this;
		}

		auto operator=(std::nullptr_t) noexcept -> Task& {

			Task{ nullptr }.swap(*this);
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

		auto swap(Task& other) noexcept -> void {

			std::swap(handle, other.handle);
		}

		~Task() {

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