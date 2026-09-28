#pragma once

#include <winrt/Windows.Networking.Connectivity.h>

namespace Citrine {

	using NetworkConnectionProfile = winrt::Windows::Networking::Connectivity::ConnectionProfile;
	using NetworkConnectivityChangedEventHandler = winrt::delegate<NetworkConnectionProfile>;

	class NetworkInfoService {
	public:

		static auto GetConnectionProfile() -> NetworkConnectionProfile;
		static auto ConnectivityChanged(NetworkConnectivityChangedEventHandler const& handler) -> winrt::event_token;
		static auto ConnectivityChanged(winrt::event_token token) -> void;
	};
}
