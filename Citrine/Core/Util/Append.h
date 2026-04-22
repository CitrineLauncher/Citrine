#pragma once

namespace Citrine {

	template<typename T>
	struct AppendTo {

		constexpr explicit AppendTo(T& value) noexcept
			
			: Value(value)
		{}

		constexpr auto operator->() const noexcept -> T* {

			return &Value;
		}

		T& Value;
	};
}