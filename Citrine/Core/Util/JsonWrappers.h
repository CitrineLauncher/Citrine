#pragma once

#include <type_traits>
#include <functional>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine::Glaze {

	template<typename T>
	concept DefaultSkippable = glz::nullable_t<T> || std::is_arithmetic_v<T> || std::is_enum_v<T> || requires(T t) { t.empty(); } || requires(T t) { t.IsEmpty(); };

	template<typename ValueType>
		requires DefaultSkippable<std::remove_reference_t<ValueType>>
	struct SkipDefaultT {

		constexpr explicit operator bool() const noexcept {

			using T = std::remove_reference_t<ValueType>;

			if constexpr (glz::nullable_t<T>) {

				return Val && SkipDefaultT<decltype(*Val)>{ *Val };
			}
			else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {

				return static_cast<bool>(Val);
			}
			else if constexpr (requires(T t) { t.empty(); }) {

				return !Val.empty();
			}
			else {

				return !Val.IsEmpty();
			}
		}

		template<typename Self>
		constexpr auto operator*(this Self&& self) noexcept -> decltype(auto) {

			return std::forward_like<Self&>(self.Val);
		}

		ValueType Val;
	};

	template<auto M>
	inline constexpr auto SkipDefault = [](auto& self) static {
		
		using ValueType = std::invoke_result_t<decltype(M), decltype(self)>;
		return SkipDefaultT<ValueType>{ std::invoke(M, self) };
	};

	template<typename T, auto C>
	concept ConditionallySkippable = glz::nullable_t<T> || std::is_invocable_r_v<bool, decltype(C), T const&> || std::same_as<bool, decltype(C)>;

	template<typename ValueType, auto C>
		requires ConditionallySkippable<std::remove_reference_t<ValueType>, C>
	struct SkipIfT {

		constexpr explicit operator bool() const noexcept {

			using T = std::remove_reference_t<ValueType>;

			if constexpr (glz::nullable_t<T>) {

				return Val && SkipIfT<decltype(*Val), C>{ *Val };
			}
			else if constexpr (std::is_invocable_r_v<bool, decltype(C), T const&>) {

				return !std::invoke(C, Val);
			}
			else {

				return !C;
			}
		}

		template<typename Self>
		constexpr auto operator*(this Self&& self) noexcept -> decltype(auto) {

			return std::forward_like<Self&>(self.Val);
		}

		ValueType Val;
	};

	template<auto M, auto C>
	inline constexpr auto SkipIf = [](auto& self) static {
		
		using ValueType = std::invoke_result_t<decltype(M), decltype(self)>;
		return SkipIfT<ValueType, C>{ std::invoke(M, self) };
	};
}

namespace glz {

	using ::Citrine::Glaze::SkipDefault;

	template<typename T>
	struct from<JSON, ::Citrine::Glaze::SkipDefaultT<T>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) -> void {

			parse<JSON>::op<Opts>(value.Val, args...);
		}
	};

	template<typename T>
	struct to<JSON, ::Citrine::Glaze::SkipDefaultT<T>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) noexcept -> void {

			serialize<JSON>::op<Opts>(value.Val, args...);
		}
	};

	using ::Citrine::Glaze::SkipIf;

	template<typename T, auto C>
	struct from<JSON, ::Citrine::Glaze::SkipIfT<T, C>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) -> void {

			parse<JSON>::op<Opts>(value.Val, args...);
		}
	};

	template<typename T, auto C>
	struct to<JSON, ::Citrine::Glaze::SkipIfT<T, C>> {

		template<auto Opts>
		static auto op(auto&& value, auto&&... args) noexcept -> void {

			serialize<JSON>::op<Opts>(value.Val, args...);
		}
	};
}