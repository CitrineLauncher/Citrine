#pragma once

#include "Core/Coroutine/TaskPromise.h"
#include "Core/Coroutine/SignalAwaiter.h"

#include <memory>
#include <mutex>
#include <optional>

namespace Citrine {

	class ServiceImplBase {
	protected:

		template<typename T>
		class Operation;

		class OperationCollection;

		template<typename OperationT>
		class OperationHandle;

		class OperationTaskBase {
		protected:

			static auto CreateCompletedEvent() noexcept -> void*;
			static auto SetCompletedEvent(void* event) noexcept -> void;
			static auto ReleaseCompletedEvent(void* event) noexcept -> void;
		};

		template<typename T>
		class OperationTask : public OperationTaskBase {
		public:

			struct promise_type : public TaskPromise<T> {

				auto Start() noexcept -> void {

					TaskPromiseBase::Start();
					if (completedEvent.load(std::memory_order::acquire) == CompletedEventState::Completed)
						return;

					auto expected = static_cast<void*>(nullptr);
					auto event = CreateCompletedEvent();

					if (!completedEvent.compare_exchange_strong(expected, event, std::memory_order::release, std::memory_order::acquire))
						ReleaseCompletedEvent(event);
				}

				auto OnFinalSuspend() noexcept -> void {

					auto event = static_cast<void*>(nullptr);

					if (!completedEvent.compare_exchange_strong(event, CompletedEventState::Completed, std::memory_order::release, std::memory_order::acquire))
						SetCompletedEvent(event);
				}

				auto GetCompletedEvent() noexcept -> void* {

					auto event = completedEvent.load(std::memory_order::relaxed);
					return (event > CompletedEventState::Completed) ? event : nullptr;
				}

				~promise_type() noexcept {

					auto event = completedEvent.load(std::memory_order::relaxed);
					if (event > CompletedEventState::Completed)
						ReleaseCompletedEvent(event);
				}

			private:
				
				using StateSentinel = PromiseBase::StateSentinel;

				struct CompletedEventState {

					static constexpr auto Completed = StateSentinel{ 1 };
				};

				std::atomic<void*> completedEvent{ nullptr };
			};

			OperationTask() noexcept = default;

			OperationTask(std::nullptr_t) noexcept : handle(nullptr) {}

			OperationTask(std::coroutine_handle<promise_type> handle) noexcept

				: handle(handle)
			{
				handle.promise().Start();
			}

			OperationTask(OperationTask const&) = delete;
			auto operator=(OperationTask const&) = delete;

			OperationTask(OperationTask&& other) noexcept

				: handle(std::exchange(other.handle, nullptr))
			{}

			auto operator=(OperationTask&& other) noexcept -> OperationTask& {

				OperationTask{ std::move(other) }.swap(*this);
				return *this;
			}

			auto operator=(std::nullptr_t) noexcept -> OperationTask& {

				OperationTask{ nullptr }.swap(*this);
				return *this;
			}

			explicit operator bool() const noexcept {

				return bool{ handle };
			}

			auto swap(OperationTask& other) noexcept -> void {

				std::swap(handle, other.handle);
			}

			~OperationTask() {

				Abandon();
			}

		private:

			template<typename T>
			friend class Operation;

			auto Abandon() noexcept -> void {

				if (handle) {

					handle.promise().Cancel();
					handle.promise().Abandon();
				}
			}

			std::coroutine_handle<promise_type> handle{ nullptr };
		};

		class OperationBase {
		public:

			virtual ~OperationBase() = default;

		protected:

			friend OperationCollection;

			template<typename OperationT>
			friend class OperationHandle;

			OperationBase() noexcept = default;

			OperationBase(OperationBase const&) = delete;
			auto operator=(OperationBase const&) = delete;

 			auto Capture() noexcept -> void {

				++refCount;
			}

			auto Release() noexcept -> bool {

				return --refCount == 0;
			}

			std::uint64_t refCount{ 0 };
			OperationCollection* parent{ nullptr };
		};

		template<typename T>
		class Operation : public OperationBase {
		public:

			using ResultType = T;
			using TaskType = OperationTask<ResultType>;

			class Awaitable {
			public:

				using TimeoutDuration = SignalAwaiter::TimeoutDuration;

				Awaitable(std::coroutine_handle<typename TaskType::promise_type> handle, TimeoutDuration timeout)

					: handle(handle)
				{
					if (auto event = this->handle.promise().GetCompletedEvent())
						signalAwaiter.emplace(event, timeout);
				}

				auto await_ready() const noexcept -> bool {

					return !signalAwaiter || signalAwaiter->await_ready();
				}

				template<typename Promise>
				auto await_suspend(std::coroutine_handle<Promise> continuation) -> bool {

					return signalAwaiter->await_suspend(continuation);
				}

				auto await_resume() -> decltype(auto) {

					if (signalAwaiter) try {

						if (!signalAwaiter->await_resume())
							throw TaskTimeoutException{};
					}
					catch (winrt::hresult_canceled const&) {

						throw TaskCancelledException{};
					}
					return handle.promise().GetResult();
				}

			private:

				std::coroutine_handle<typename TaskType::promise_type> handle{ nullptr };
				std::optional<SignalAwaiter> signalAwaiter;
			};

		private:

			friend OperationCollection;

			template<typename OperationT>
			friend class OperationHandle;

			auto GetAwaiter(Awaitable::TimeoutDuration timeout = {}) const noexcept -> Awaitable {

				return Awaitable{ task.handle, timeout };
			}

			OperationTask<T> task;
		};

		class OperationCollection {
		public:

			OperationCollection() noexcept = default;

			OperationCollection(OperationCollection const&) = delete;
			auto operator=(OperationCollection const&) = delete;

			class LockT {
			public:

				LockT(LockT const&) = delete;
				auto operator=(LockT const&) = delete;

				auto begin() const noexcept -> auto {

					return std::as_const(collection->operations).begin();
				}

				auto end() const noexcept -> auto {

					return std::as_const(collection->operations).end();
				}

				template<typename OperationT, typename T = OperationT::ResultType>
				auto Add(std::unique_ptr<OperationT> op, OperationTask<T>&& task) -> auto {

					auto& operations = collection->operations;

					auto operation = static_cast<Operation<T>*>(operations.emplace_back() = op.release());
					operation->parent = collection;
					operation->task = std::move(task);

					return static_cast<OperationT*>(operation);
				}

				~LockT() noexcept {

					auto& operations = collection->operations;
					auto it = operations.begin();

					while (it != operations.end()) {

						auto operation = *it;
						if (operation->refCount == 0) {

							delete operation;
							it = operations.erase(it);
						}
						else {

							++it;
						}
					}

					collection->mutex.unlock();
				}

			private:

				friend OperationCollection;

				LockT(OperationCollection* collection) noexcept

					: collection(collection)
				{
					collection->mutex.lock();
				}

				OperationCollection* collection;
			};

			auto Lock() noexcept -> LockT {

				return this;
			}

		private:

			template<typename OperationT>
			friend class OperationHandle;

			std::recursive_mutex mutex;
			std::vector<OperationBase*> operations;
		};

		template<typename OperationT>
		class OperationHandle {
		public:

			OperationHandle() = default;

			OperationHandle(std::nullptr_t) noexcept : operation(nullptr) {}

			OperationHandle(OperationHandle const& other) noexcept

				: operation(other.operation)
			{
				if (!operation)
					return;

				auto collection = operation->parent;
				auto lock = std::scoped_lock{ collection->mutex };

				operation->Capture();
			}

			auto operator=(OperationHandle const& other) noexcept -> OperationHandle& {
			
				OperationHandle{ other }.swap(*this);
				return *this;
			};

			OperationHandle(OperationHandle&& other) noexcept

				: operation(std::exchange(other.operation, nullptr))
			{}

			auto operator=(OperationHandle&& other) noexcept -> OperationHandle& {

				OperationHandle{ std::move(other) }.swap(*this);
			};

			auto operator=(std::nullptr_t) noexcept -> OperationHandle& {

				OperationHandle{ nullptr }.swap(*this);
				return *this;
			}

			explicit operator bool() const noexcept {

				return static_cast<bool>(operation);
			}

			auto Attach(OperationT* op) -> void {

				OperationHandle{ op }.swap(*this);
			}

			auto operator co_await() const -> decltype(auto) {

				return operation->GetAwaiter();
			}

			auto WithTimeout(OperationT::Awaitable::TimeoutDuration timeout) const -> decltype(auto) {

				return operation->GetAwaiter(timeout);
			}

			auto swap(OperationHandle& other) noexcept -> void {

				std::swap(operation, other.operation);
			}

			~OperationHandle() noexcept {

				if (!operation)
					return;

				auto collection = operation->parent;
				auto lock = std::scoped_lock{ collection->mutex };

				if (operation->Release()) {

					auto& operations = collection->operations;
					auto it = std::ranges::find(operations, operation);

					delete operation;
					if (it != operations.end())
						operations.erase(it);
				}
			}

		private:

			OperationHandle(OperationT* op) noexcept

				: operation(op)
			{
				operation->Capture();
			}

			OperationT* operation{ nullptr };
		};
	};
}