#pragma once

#include <type_traits>

namespace Citrine {

	template<auto>
	struct MemberPointerClass;

	template<typename T, typename C, T C::* V>
	struct MemberPointerClass<V> {

		using type = C;
	};

	template<auto V>
	using MemberPointerClassT = MemberPointerClass<V>::type;

	template<auto>
	struct MemberPointerType;

	template<typename T, typename C, T C::* V>
	struct MemberPointerType<V> {

		using type = T;
	};

	template<auto V>
	using MemberPointerTypeT = MemberPointerType<V>::type;
}