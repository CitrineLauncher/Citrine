#pragma once

#include <type_traits>
#include <algorithm>

namespace Citrine {

	template<typename T, std::size_t N>
		requires std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>
	class TrivialArray {
	public:

		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = pointer;
		using const_iterator = const_pointer;

#pragma warning(push)
#pragma warning(disable : 26495) // Always initialize a member variable

		constexpr TrivialArray() noexcept {

			if consteval {

				std::ranges::fill(elements, T{});
			}
		}

#pragma warning(pop)

		constexpr auto operator[](size_type index) noexcept -> reference {

			return elements[index];
		}

		constexpr auto operator[](size_type index) const noexcept -> const_reference {

			return elements[index];
		}

		constexpr auto data() noexcept -> pointer {

			return elements;
		}

		constexpr auto data() const noexcept -> const_pointer {

			return elements;
		}

		constexpr auto begin() noexcept -> iterator {

			return elements;
		}

		constexpr auto begin() const noexcept -> const_iterator {

			return elements;
		}

		constexpr auto end() noexcept -> iterator {

			return elements + N;
		}

		constexpr auto end() const noexcept -> const_pointer {

			return elements + N;
		}

		static constexpr auto empty() noexcept -> bool {

			return N == 0;
		}

		static constexpr auto size() noexcept -> size_type {

			return N;
		}

	private:

		value_type elements[N > 0 ? N : 1];
	};
}