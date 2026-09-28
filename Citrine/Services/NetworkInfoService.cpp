#include "pch.h"
#include "NetworkInfoService.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::Networking::Connectivity;
}

namespace {

	auto connectivityChangedEvent = winrt::event<NetworkConnectivityChangedEventHandler>{};
	auto networkStatusChangedToken = winrt::NetworkInformation::NetworkStatusChanged([](auto const&) static {

		try {

			connectivityChangedEvent(winrt::NetworkInformation::GetInternetConnectionProfile());
		}
		catch (winrt::hresult_error const&) {
		
			connectivityChangedEvent(nullptr);
		}
	});
}

namespace Citrine {

	auto NetworkInfoService::GetConnectionProfile() -> NetworkConnectionProfile try {

		return winrt::NetworkInformation::GetInternetConnectionProfile();
	}
	catch (winrt::hresult_error const&) {

		return nullptr;
	}

	auto NetworkInfoService::ConnectivityChanged(NetworkConnectivityChangedEventHandler const& handler) -> winrt::event_token {

		return connectivityChangedEvent.add(handler);
	}

	auto NetworkInfoService::ConnectivityChanged(winrt::event_token token) -> void {

		connectivityChangedEvent.remove(token);
	}
}