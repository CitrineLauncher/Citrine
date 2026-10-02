#pragma once

#include <Core/Net/NetworkInfo.h>

#include <winrt/base.h>

namespace Citrine {

	using NetworkConnectivityChangedEventHandler = winrt::delegate<NetworkConnectivity>;
	using NetworkConnectionCostChangedEventHandler = winrt::delegate<NetworkConnectionCost>;

	class NetworkInfoService {
	public:

		static auto GetNetworkConnectivity() -> NetworkConnectivity;
		static auto NetworkConnectivityChanged(NetworkConnectivityChangedEventHandler const& handler) -> winrt::event_token;
		static auto NetworkConnectivityChanged(winrt::event_token token) -> void;

		static auto GetNetworkConnectionCost() -> NetworkConnectionCost;
		static auto NetworkConnectionCostChanged(NetworkConnectionCostChangedEventHandler const& handler) -> winrt::event_token;
		static auto NetworkConnectionCostChanged(winrt::event_token token) -> void;
	};
}
