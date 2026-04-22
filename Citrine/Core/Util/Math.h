#pragma once

#include <concepts>
#include <type_traits>
#include <limits>

namespace Citrine {

	template<std::integral To, std::integral From>
	constexpr auto SaturatingCast(From from) noexcept -> To {

		using Limits = std::numeric_limits<To>;

		if constexpr (std::is_signed_v<To> != std::is_signed_v<From>) {

			if constexpr (std::is_signed_v<To>) {

				if constexpr (sizeof(To) <= sizeof(From)) {

					if (from > static_cast<From>(Limits::max()))
						return Limits::max();
				}
			}
			else {

				if (from < 0)
					from = 0;

				if constexpr (sizeof(To) < sizeof(From)) {

					if (from > static_cast<From>(Limits::max()))
						return Limits::max();
				}
			}
		}
		else if constexpr (sizeof(To) < sizeof(From)) {

			if constexpr (std::is_signed_v<To>) {

				if (from < static_cast<From>(Limits::min()))
					return Limits::min();
			}

			if (from > static_cast<From>(Limits::max()))
				return Limits::max();
		}

		return static_cast<To>(from);
	}
}