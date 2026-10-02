#pragma once

#include "Core/Util/Concepts.h"

#include <coroutine>

namespace Citrine {

	template<typename T>
	concept Awaiter = requires(T t, std::coroutine_handle<> h) {

		{ t.await_ready() } -> std::same_as<bool>;
		{ t.await_suspend(h) } -> IsAnyOf<void, bool, std::coroutine_handle<>>;
		{ t.await_resume() };
	};

	template<typename T>
	concept MemberAwaitable = requires(T t) {

		{ std::move(t).operator co_await() } -> Awaiter;
	};

	template<typename T>
	concept NonMemberAwaitable = requires(T t) {

		{ operator co_await(std::move(t)) } -> Awaiter;
	};

	template<typename T>
	concept Awaitable = MemberAwaitable<T> || NonMemberAwaitable<T> || Awaiter<T>;
}