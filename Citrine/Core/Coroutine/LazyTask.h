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

		class Awaitable : public TaskAwaiterBase<T>{
		public:

			using TaskAwaiterBase<T>::TaskAwaiterBase;

			auto await_ready() const noexcept -> bool {

				return this->GetPromise().AwaitReady();
			}

			template<typename Promise>
			auto await_suspend(std::coroutine_handle<Promise> continuation) noexcept -> bool {

				if (auto cancelled = this->SetCanceller(continuation))
					return false;

				this->GetPromise().Start();

				if (this->GetPromise().AwaitReady())
					return false;

				return this->GetPromise().AwaitSuspend(continuation);
			}

			auto await_resume() -> decltype(auto) {

				return this->GetPromise().GetResult();
			}
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

		template<typename>
		friend class Task;

		auto Abandon() noexcept -> void {

			if (handle)
				handle.promise().Abandon();
		}

		std::coroutine_handle<promise_type> handle{ nullptr };
	};
}