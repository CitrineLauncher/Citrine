#pragma once

#include <memory>
#include <utility>
#include <algorithm>
#include <concepts>
#include <type_traits>
#include <stdexcept>

namespace Citrine {

	class EventDelegateBase;
	class EventDelegateListBase;

	class EventToken {
	public:

		constexpr EventToken() noexcept = default;

		EventToken(EventToken const&) = delete;
		auto operator=(EventToken const&) = delete;

		EventToken(EventToken&& other) noexcept
			
			: delegate(std::exchange(other.delegate, nullptr))
		{}

		auto operator=(EventToken&& other) noexcept -> EventToken& {

			EventToken{ std::move(other) }.swap(*this);
			return *this;
		}

		explicit operator bool() noexcept {

			return delegate;
		}

		auto swap(EventToken& other) noexcept -> void {

			std::swap(delegate, other.delegate);
		}

	private:

		EventToken(EventDelegateBase* delegate) noexcept

			: delegate(delegate)
		{}

		friend class EventDelegateListBase;
		template<typename...>
		friend class EventDelegateList;
		friend class EventRevoker;

		EventDelegateBase* delegate{ nullptr };
	};

	class EventDelegateBase {
	public:

		virtual ~EventDelegateBase() = default;

	protected:

		EventDelegateBase() noexcept = default;

		EventDelegateBase(EventDelegateBase const&) = delete;
		auto operator=(EventDelegateBase const&) = delete;

	private:

		friend class EventDelegateListBase;
		template<typename...>
		friend class EventDelegateList;
		friend class EventRevoker;

		EventDelegateListBase* parent{ nullptr };
	};

	template<typename... Args>
	class EventDelegate : public EventDelegateBase {
	public:

		virtual auto Invoke(Args const&... args) const -> void = 0;
		virtual auto Clone() const -> std::unique_ptr<EventDelegate> = 0;

	protected:

		EventDelegate() noexcept = default;

		EventDelegate(EventDelegate const&) = delete;
		auto operator=(EventDelegate const&) = delete;
	};

	class EventDelegateListBase {
	public:

		auto Remove(EventToken&& token) noexcept -> void {

			auto it = std::ranges::find(myFirst, myLast, token.delegate);
			if (it == myLast)
				return;

			std::ranges::copy(it + 1, myLast--, it);
			if (it < nextDelegate && it == --nextDelegate) {

				std::exchange(token.delegate, nullptr)->parent = nullptr;
				return;
			}

			delete std::exchange(token.delegate, nullptr);
		}

		~EventDelegateListBase() noexcept {

			for (auto it = myFirst; it < myLast; ++it)
				delete* it;
			delete[] myFirst;

			if (!revokerInfo)
				return;

			revokerInfo->Parent = nullptr;
			if (revokerInfo->Count == 0)
				delete revokerInfo;
		}

	protected:

		constexpr EventDelegateListBase() noexcept = default;

		EventDelegateListBase(EventDelegateListBase const&) = delete;
		auto operator=(EventDelegateListBase const&) = delete;

		friend class EventRevoker;

		using ValueType = EventDelegateBase*;
		struct RevokerInfo {

			EventDelegateListBase* Parent{ nullptr };
			std::size_t Count{};
		};

		auto AttachRevoker() -> RevokerInfo* {

			if (!revokerInfo)
				revokerInfo = new RevokerInfo{ this };

			++revokerInfo->Count;
			return revokerInfo;
		}

		struct PositionGuard {

			~PositionGuard() noexcept {

				position = nullptr;
			}

			ValueType*& position;
		};

		struct DelegateGuard {

			~DelegateGuard() noexcept {

				if (!delegate->parent)
					delete delegate;
			}

			ValueType& delegate;
		};

		mutable ValueType* nextDelegate{ nullptr }; // We can't store the current position due to pointer arithmetic
		ValueType* myFirst{ nullptr };
		ValueType* myLast{ nullptr };
		ValueType* myEnd{ nullptr };
		RevokerInfo* revokerInfo{ nullptr };
	};

	template<typename... Args>
	class EventDelegateList : public EventDelegateListBase {
	public:

		constexpr EventDelegateList() noexcept = default;

		EventDelegateList(EventDelegateList const&) = delete;
		auto operator=(EventDelegateList const&) = delete;

		auto Add(EventDelegate<Args...> const& delegate) -> EventToken {

			return Add(delegate.Clone());
		}

		auto Add(std::unique_ptr<EventDelegate<Args...>>&& delegate) -> EventToken {

			if (nextDelegate)
				throw std::runtime_error{ "Cannot add handlers during event invocation" };

			if (myLast == myEnd) {

				auto oldCapacity = static_cast<std::size_t>(myEnd - myFirst);
				auto newCapacity = std::max(oldCapacity + oldCapacity / 2, 8uz);

				auto newData = new ValueType[newCapacity];
				std::ranges::copy(myFirst, myEnd, newData);
				delete[] myFirst;

				myFirst = newData;
				myLast = newData + oldCapacity;
				myEnd = newData + newCapacity;
			}

			auto currentDelegate = (*myLast++ = delegate.release());
			currentDelegate->parent = this;
			return currentDelegate;
		}

		auto Invoke(Args const&... args) const -> void {

			auto positionGuard = PositionGuard{ nextDelegate };
			nextDelegate = myFirst;

			while (nextDelegate < myLast) {

				auto currentDelegate = *nextDelegate++;
				auto delegateGuard = DelegateGuard{ currentDelegate };
				static_cast<EventDelegate<Args...> const&>(*currentDelegate).Invoke(args...);
			}
		}
	};

	template<typename... Args>
	class EventHandler {
	public:

		constexpr EventHandler() noexcept = default;

		template<std::copy_constructible F>
		EventHandler(F&& func)
			
			: delegate(new Delegate<F>{ std::forward<F>(func) })
		{}

		template<typename O, std::copy_constructible F>
		EventHandler(O* object, F&& func)

			: delegate(new Delegate<O*, F>{ std::move(object), std::forward<F>(func) })
		{}

		template<typename O, std::copy_constructible F>
		EventHandler(std::shared_ptr<O> object, F&& func)

			: delegate(new Delegate<std::shared_ptr<O>, F>{ std::move(object), std::forward<F>(func) })
		{}

		template<typename O, std::copy_constructible F>
		EventHandler(std::weak_ptr<O> object, F&& func)

			: delegate(new Delegate<std::weak_ptr<O>, F>{ std::move(object), std::forward<F>(func) })
		{}

		EventHandler(EventHandler const& other) 

			: delegate(other.delegate->Clone())
		{}

		auto operator=(EventHandler const& other) -> EventHandler& {

			if (this != &other) {

				delegate = other.delegate->Clone();
			}
			return *this;
		}

		EventHandler(EventHandler&&) noexcept = default;
		auto operator=(EventHandler&&) noexcept -> EventHandler& = default;

		explicit operator bool() const noexcept {

			return static_cast<bool>(delegate);
		}

		auto Invoke(Args const&... args) const -> void {

			delegate->Invoke(args...);
		}

		auto operator()(Args const&... args) const -> void {

			Invoke(args...);
		}

		auto swap(EventHandler& other) noexcept -> void {

			delegate.swap(other.delegate);
		}

	private:

		template<typename>
		friend class Event;

		template<typename...>
		class Delegate;

		template<typename Func>
		class Delegate<Func> final : public EventDelegate<Args...> {
		public:

			template<typename F>
			Delegate(F&& func)
				
				: func(std::forward<F>(func))
			{}

			auto Invoke(Args const&... args) const -> void override {

				std::invoke(func, args...);
			}

			auto Clone() const -> std::unique_ptr<EventDelegate<Args...>> override {

				return std::make_unique<Delegate>(func);
			}

			[[no_unique_address, msvc::no_unique_address]]
			mutable Func func;
		};

		template<typename ObjPtr, typename Func>
		class Delegate<ObjPtr, Func> final : public EventDelegate<Args...> {
		public:

			template<typename F>
			Delegate(ObjPtr&& object, F&& func)
				
				: object(std::move(object))
				, func(std::forward<F>(func))
			{}

			auto Invoke(Args const&... args) const -> void override {

				if constexpr (requires{ *object; }) {

					std::invoke(func, *object, args...);
				}
				else if (auto sharedObject = object.lock()) {

					std::invoke(func, *sharedObject, args...);
				}
			}

			auto Clone() const -> std::unique_ptr<EventDelegate<Args...>> override {

				return std::make_unique<Delegate>(object, func);
			}

			ObjPtr object{ nullptr };
			[[no_unique_address, msvc::no_unique_address]]
			mutable Func func;
		};

		std::unique_ptr<EventDelegate<Args...>> delegate;
	};

	template<typename>
	class Event;

	template<typename... Args>
	class Event<EventHandler<Args...>> {
	public:

		constexpr Event() noexcept = default;

		Event(Event const&) = delete;
		auto operator=(Event const&) = delete;

		auto Add(EventHandler<Args...> const& handler) -> EventToken {

			return delegates.Add(*handler.delegate);
		}

		auto Add(EventHandler<Args...>&& handler) -> EventToken {

			return delegates.Add(std::move(handler.delegate));
		}

		auto Remove(EventToken&& token) noexcept -> void {

			delegates.Remove(std::move(token));
		}

		auto Invoke(Args const&... args) const -> void {

			delegates.Invoke(args...);
		}

		auto operator()(Args const&... args) const -> void {

			Invoke(args...);
		}

	private:

		auto Initialize() -> void {

			delegates = std::make_shared<EventDelegateList<Args...>>();
		}

		EventDelegateList<Args...> delegates;
	};

	class EventRevoker {
	public:

		constexpr EventRevoker() noexcept = default;

		EventRevoker(EventRevoker const&) = delete;
		auto operator=(EventRevoker const&) = delete;

		EventRevoker(EventRevoker&& other) noexcept

			: delegate(std::exchange(other.delegate, nullptr))
			, revokerInfo(std::exchange(other.revokerInfo, nullptr))
		{}

		auto operator=(EventRevoker&& other) noexcept -> EventRevoker& {

			EventRevoker{ std::move(other) }.swap(*this);
			return *this;
		}

		EventRevoker(EventToken&& token) noexcept {

			if (!token)
				return;

			delegate = std::exchange(token.delegate, nullptr);
			revokerInfo = delegate->parent->AttachRevoker();
		}

		auto operator=(EventToken&& token) noexcept -> EventRevoker& {

			Revoke();
			if (!token)
				return *this;

			delegate = std::exchange(token.delegate, nullptr);
			revokerInfo = delegate->parent->AttachRevoker();
			return *this;
		}

		explicit operator bool() noexcept {

			return static_cast<bool>(delegate);
		}

		auto Revoke() noexcept -> void {

			if (!delegate)
				return;

			auto delegates = revokerInfo->Parent;
			auto newCount = --revokerInfo->Count;

			if (!delegates) {

				if (newCount == 0)
					delete revokerInfo;
			}
			else {

				delegates->Remove(delegate);
			}

			delegate = nullptr;
			revokerInfo = nullptr;
		}

		auto swap(EventRevoker& other) noexcept -> void {

			std::swap(delegate, other.delegate);
			std::swap(revokerInfo, other.revokerInfo);
		}

		~EventRevoker() noexcept {

			Revoke();
		}

	private:

		EventDelegateBase* delegate{ nullptr };
		EventDelegateListBase::RevokerInfo* revokerInfo{ nullptr };
	};
}