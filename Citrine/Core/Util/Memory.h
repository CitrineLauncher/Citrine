#pragma once

#include <cstdint>
#include <cstddef>

namespace Citrine {

	// Temporary workaround until std::start_lifetime_as and std::start_lifetime_as_array become available

	template<typename T>
	[[nodiscard]] auto StartLifetimeAs(void* mem) noexcept -> T* {

		auto bytes = new (mem) std::uint8_t[sizeof(T)];
		auto ptr = reinterpret_cast<T*>(bytes);
		static_cast<void>(*ptr);
		return ptr;
	}

	template<typename T>
	[[nodiscard]] auto StartLifetimeAs(void const* mem) noexcept -> T const* {

		auto bytes = new (const_cast<void*>(mem)) std::uint8_t[sizeof(T)];
		auto ptr = reinterpret_cast<T const*>(bytes);
		static_cast<void>(*ptr);
		return ptr;
	}

	template<typename T>
	[[nodiscard]] auto StartLifetimeAsArray(void* mem, std::size_t n) noexcept -> T* {

		auto bytes = new (mem) std::uint8_t[sizeof(T) * n];
		auto ptr = reinterpret_cast<T*>(bytes);
		static_cast<void>(*ptr);
		return ptr;
	}

	template<typename T>
	[[nodiscard]] auto StartLifetimeAsArray(void const* mem, std::size_t n) noexcept -> T const* {

		auto bytes = new (const_cast<void*>(mem)) std::uint8_t[sizeof(T) * n];
		auto ptr = reinterpret_cast<T const*>(bytes);
		static_cast<void>(*ptr);
		return ptr;
	}
}