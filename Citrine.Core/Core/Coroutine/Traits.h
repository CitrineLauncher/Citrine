#pragma once

#include "Concepts.h"

namespace Citrine {

	template<Awaitable A>
	auto GetAwaiter(A&& awaitable) -> decltype(auto) {

		if constexpr (MemberAwaitable<A>) {

			return std::forward<A>(awaitable).operator co_await();
		}
		else if constexpr (NonMemberAwaitable<A>) {

			return operator co_await(std::forward<A>(awaitable));
		}
		else {

			return std::forward<A>(awaitable);
		}
	}

	template<Awaitable A, typename = void>
	struct AwaitableTraits {};

	template<Awaitable A>
	struct AwaitableTraits<A> {

		using Awaiter = decltype(GetAwaiter(std::declval<A>()));
		using AwaiterResult = decltype(std::declval<Awaiter>().await_resume());
	};
}