#pragma once

#include "Concepts.h"

namespace Citrine {

	template<Awaitable AwaitableT>
	auto GetAwaiter(AwaitableT&& awaitable) -> decltype(auto) {

		if constexpr (MemberAwaitable<AwaitableT>) {

			return std::forward<AwaitableT>(awaitable).operator co_await();
		}
		else if constexpr (NonMemberAwaitable<AwaitableT>) {

			return operator co_await(std::forward<AwaitableT>(awaitable));
		}
		else {

			return std::forward<AwaitableT>(awaitable);
		}
	}

	template<Awaitable AwaitableT, typename = void>
	struct AwaitableTraits {};

	template<Awaitable AwaitableT>
	struct AwaitableTraits<AwaitableT> {

		using Awaiter = decltype(GetAwaiter(std::declval<AwaitableT>()));
		using AwaiterResult = decltype(std::declval<Awaiter>().await_resume());
	};
}