#pragma once

#include <type_traits>
#include <concepts>

namespace Citrine {

	template<typename T, typename... Ts>
	concept IsAnyOf = (std::same_as<T, Ts> || ...);

	template<typename T>
	concept StandardLayoutType = std::is_standard_layout_v<T>;

	template<typename T>
	concept MemberPointer = std::is_member_pointer_v<T>;

	template<typename T>
	concept MemberFunctionPointer = std::is_member_function_pointer_v<T>;

	template<typename T>
	concept MemberObjectPointer = std::is_member_object_pointer_v<T>;
}
