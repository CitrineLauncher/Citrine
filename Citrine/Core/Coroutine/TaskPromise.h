#pragma once

#include <cstdint>
#include <variant>
#include <stdexcept>
#include <atomic>
#include <winrt/base.h>

namespace Citrine {

	template<typename T>
	class TaskPromise;

	class TaskCancelledException : public std::runtime_error {
	public:

		explicit TaskCancelledException() noexcept : runtime_error("Task cancelled") {}
	};

	struct GetCancellationTokenT {};

	consteval auto GetCancellationToken() noexcept -> GetCancellationTokenT {

		return {};
	}

	class TaskCancellationCallback {
	public:

		constexpr TaskCancellationCallback() noexcept = default;
		constexpr TaskCancellationCallback(std::nullptr_t) noexcept {}

		template<std::move_constructible F>
		TaskCancellationCallback(F&& func)

			: delegate(new Delegate<F>{ std::forward<F>(func) })
		{}

		TaskCancellationCallback(TaskCancellationCallback const&) = delete;
		auto operator=(TaskCancellationCallback const&) = delete;

		TaskCancellationCallback(TaskCancellationCallback&&) noexcept = default;
		auto operator=(TaskCancellationCallback&&) noexcept -> TaskCancellationCallback& = default;

	private:

		friend class TaskPromiseBase;

		class IDelegate {
		public:

			virtual auto Invoke() const -> void = 0;
			virtual ~IDelegate() = default;
		};

		template<typename Func>
		class Delegate : public IDelegate {
		public:

			template<typename F>
			Delegate(F&& func)

				: func(std::forward<F>(func))
			{}

			auto Invoke() const -> void override try {

				std::invoke(func);
			}
			catch (...) {

				std::terminate();
			}

			[[no_unique_address, msvc::no_unique_address]]
			mutable Func func;
		};

		std::unique_ptr<IDelegate> delegate;
	};

	class TaskPromiseBase : public winrt::cancellable_promise {
	public:

		TaskPromiseBase() noexcept {

			enable_cancellation_propagation(true);
		}

		TaskPromiseBase(TaskPromiseBase const&) = delete;
		auto operator=(TaskPromiseBase const&) = delete;

		template<typename T>
		auto get_return_object(this TaskPromise<T>& self) noexcept -> std::coroutine_handle<TaskPromise<T>> {

			return std::coroutine_handle<TaskPromise<T>>::from_promise(self);
		}

		auto initial_suspend() noexcept -> std::suspend_always { return {}; }

		using ContinuationHandler = auto(*)(void*) -> void;

		struct FinalAwaitable : std::suspend_always {

			template<typename T>
			auto await_suspend(std::coroutine_handle<TaskPromise<T>> handle) noexcept -> void {

				auto& promise = handle.promise();
				auto continuation = promise.state.exchange(State::Completed, std::memory_order::acq_rel);

				if (continuation == State::Abandoned) {

					handle.destroy();
				}
				else if (continuation != State::Running) {

					if (promise.useCustomContinuationHandler)
						reinterpret_cast<ContinuationHandler>(continuation)(promise.continuationParameter);
					else if (promise.context)
						promise.ResumeInContext(continuation);
					else
						std::coroutine_handle<>::from_address(continuation)();
				}
			}
		};

		auto final_suspend() noexcept -> FinalAwaitable { return {}; }

		template<typename Expression>
		auto await_transform(Expression&& expression) -> Expression&& {

			if (IsCancelled())
				throw TaskCancelledException{};

			return std::forward<Expression>(expression);
		}

		template<typename T>
		auto Start(this TaskPromise<T>& self) noexcept -> void {

			self.state.store(State::Running, std::memory_order::relaxed);
			std::coroutine_handle<TaskPromise<T>>::from_promise(self).resume();
		}

		template<typename T>
		auto Start(this TaskPromise<T>& self, ContinuationHandler continuationHandler, void* continuationParam) noexcept -> void {

			self.useCustomContinuationHandler = true;
			self.continuationParameter = continuationParam;
			self.state.store(reinterpret_cast<void*>(continuationHandler), std::memory_order::relaxed);
			std::coroutine_handle<TaskPromise<T>>::from_promise(self).resume();
		}

		template<typename T>
		auto Cancel(this TaskPromise<T>& self) -> void {

			if (self.state.load(std::memory_order::relaxed) == State::Completed)
				return;

			if (self.cancelling.test_and_set(std::memory_order::seq_cst))
				return;

			self.CancelInternal();

			self.cancelling.clear(std::memory_order::release);
			self.cancelling.notify_one();
		}

		auto IsCancelled() const noexcept -> bool {

			return cancellationState.load(std::memory_order::relaxed) == CancellationState::Cancelled;
		}

		class CancellationToken {
		public:

			CancellationToken(TaskPromiseBase* promise) noexcept : promise(promise) {}

			CancellationToken(CancellationToken const&) = delete;
			auto operator=(CancellationToken const&) = delete;

			auto CancellationRequested() const noexcept -> bool {

				return promise->IsCancelled();
			}

			auto ThrowIfCancellationRequested() const -> void {

				if (promise->IsCancelled())
					throw TaskCancelledException{};
			}

			auto Callback(TaskCancellationCallback callback) -> void {

				auto& cancellationState = promise->cancellationState;
				auto oldCallback = cancellationState.load(std::memory_order::relaxed);

				if (oldCallback != CancellationState::Cancelled &&
					cancellationState.compare_exchange_strong(oldCallback, callback.delegate.get(), std::memory_order::release, std::memory_order::relaxed))
				{
					callback.delegate.release();
					delete static_cast<TaskCancellationCallback::IDelegate*>(oldCallback);
				}
				else if (callback.delegate) {

					callback.delegate->Invoke();
				}
			}

			class Awaitable : public std::suspend_never {
			public:

				Awaitable(TaskPromiseBase* promise) noexcept : promise(promise) {}

				auto await_resume() const noexcept -> CancellationToken {

					return { promise };
				}

			private:

				TaskPromiseBase* promise;
			};

		private:

			TaskPromiseBase* promise;
		};

		auto await_transform(GetCancellationTokenT) noexcept -> CancellationToken::Awaitable {

			return this;
		}

		auto ResumeAgile() noexcept -> void {

			preserveContext = false;
		}

		auto AwaitReady() const noexcept -> bool {

			return state.load(std::memory_order::relaxed) == State::Completed || IsCancelled();
		}

		auto AwaitSuspend(std::coroutine_handle<> continuation) noexcept -> bool {

			if (preserveContext)
				CaptureContext();

			return state.exchange(continuation.address(), std::memory_order::release) == State::Running;
		}

		auto AttachContinuationHandler(ContinuationHandler continuationHandler, void* continuationParam) noexcept -> bool {

			useCustomContinuationHandler = true;
			continuationParameter = continuationParam;
			return state.exchange(reinterpret_cast<void*>(continuationHandler), std::memory_order::release) == State::Running;
		}

		template<typename T>
		auto Abandon(this TaskPromise<T>& self) noexcept -> void {

			self.cancelling.wait(true, std::memory_order::acquire);
			if (self.state.exchange(State::Abandoned, std::memory_order::acq_rel) != State::Running) {

				std::coroutine_handle<TaskPromise<T>>::from_promise(self).destroy();
			}
		}

		~TaskPromiseBase() {

			if (auto callback = cancellationState.load(std::memory_order::relaxed); callback > CancellationState::Cancelled)
				delete static_cast<TaskCancellationCallback::IDelegate*>(callback);

			if (context)
				ReleaseContext();
		}

	protected:

		struct StateSentinel {

			operator void* (this StateSentinel self) noexcept {

				return reinterpret_cast<void*>(self.Value);
			}

			std::uintptr_t Value{};
		};

		struct State {

			static constexpr auto Idle = StateSentinel{ 0 };
			static constexpr auto Running = StateSentinel{ 1 };
			static constexpr auto Completed = StateSentinel{ 2 };
			static constexpr auto Abandoned = StateSentinel{ 3 };
		};

		struct CancellationState {

			static constexpr auto Cancelled = StateSentinel{ 1 };
		};

		auto CancelInternal() -> void {

			auto callback = cancellationState.exchange(CancellationState::Cancelled, std::memory_order::acquire);
			if (callback == CancellationState::Cancelled)
				return;

			if (callback > CancellationState::Cancelled) {

				auto delegate = static_cast<TaskCancellationCallback::IDelegate*>(callback);
				delegate->Invoke();
				delete delegate;
			}
			cancellable_promise::cancel();
		}

		auto CaptureContext() -> void;
		auto ResumeInContext(void* address) -> void;
		auto ReleaseContext() -> void;

		std::atomic<void*> state{ State::Idle };
		std::atomic<void*> cancellationState{ nullptr };
		std::atomic_flag cancelling{};
		bool preserveContext{ true };
		bool useCustomContinuationHandler{ false };
		void* context{ nullptr };
		void* continuationParameter{ nullptr };
	};

	template<typename T>
	class TaskPromise final : public TaskPromiseBase {
	public:

		TaskPromise() noexcept = default;

		template<typename V> requires std::is_constructible_v<T, V&&>
		auto return_value(V&& value) -> void {

			storage.template emplace<T>(std::forward<V>(value));
		}

		auto return_value(T value) -> void requires std::is_move_constructible_v<T> {

			storage.template emplace<T>(std::move(value));
		}

		auto unhandled_exception() noexcept -> void {

			auto& exception = storage.template emplace<std::exception_ptr>(std::current_exception());
			try {

				std::rethrow_exception(exception);
			}
			catch (TaskCancelledException const&) {

				CancelInternal();
			}
			catch (...) {}
		}

		auto GetResult() -> T&& {

			if (IsCancelled())
				throw TaskCancelledException{};

			std::atomic_thread_fence(std::memory_order::acquire);

			if (auto exception = std::get_if<std::exception_ptr>(&storage))
				std::rethrow_exception(*exception);

			return std::move(std::get<T>(storage));
		}

	private:

		using Storage = std::variant<std::monostate, T, std::exception_ptr>;
		Storage storage;
	};

	template<>
	class TaskPromise<void> final : public TaskPromiseBase {
	public:

		TaskPromise() noexcept = default;

		auto return_void() noexcept -> void {}

		auto unhandled_exception() noexcept -> void {

			auto& exception = storage.emplace<std::exception_ptr>(std::current_exception());
			try {
				
				std::rethrow_exception(exception);
			}
			catch (TaskCancelledException const&) {
			
				CancelInternal();
			}
			catch(...) {}
		}

		auto GetResult() -> void {

			if (IsCancelled())
				throw TaskCancelledException{};

			std::atomic_thread_fence(std::memory_order::acquire);

			if (auto exception = std::get_if<std::exception_ptr>(&storage))
				std::rethrow_exception(*exception);
		}

	private:

		using Storage = std::variant<std::monostate, std::exception_ptr>;
		Storage storage;
	};
}