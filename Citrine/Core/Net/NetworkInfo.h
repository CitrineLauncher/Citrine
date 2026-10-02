#pragma once

#include <cstdint>

namespace Citrine {

	enum struct NetworkConnectivity : std::uint8_t {

		None = 0,
		LocalAccess = 1,
		ConstrainedInternetAccess = 2,
		InternetAccess = 3
	};

	enum struct NetworkCostType : std::uint8_t {

		Unknown = 0,
		Unrestricted = 1,
		Fixed = 2,
		Variable = 3
	};

	struct NetworkConnectionCost {

		auto operator==(NetworkConnectionCost const&) const noexcept -> bool = default;

		bool ApproachingDataLimit{};
		bool BackgroundDataUsageRestricted{};
		NetworkCostType NetworkCostType{};
		bool OverDataLimit{};
		bool Roaming{};
	};
}