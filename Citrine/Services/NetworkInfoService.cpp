#include "pch.h"
#include "NetworkInfoService.h"

#include <atomic>

#include <winrt/Windows.Networking.Connectivity.h>

#include "Core/Logging/Logger.h"

using namespace Citrine;

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Networking::Connectivity;
}

namespace {

	class NetworkInfoServiceInternal {
	public:

		NetworkInfoServiceInternal() {

			networkStatusChangedToken = winrt::NetworkInformation::NetworkStatusChanged({ this, &NetworkInfoServiceInternal::OnNetworkStatusChanged });
			auto connectionProfile = GetConnectionProfile();

			auto newInfoHolder = InfoHolder{ .HasInfo = true, .Info = Info::FromConnectionProfile(connectionProfile) };
			auto emptyInfoHolder = InfoHolder{};

			infoHolder.compare_exchange_strong(emptyInfoHolder, newInfoHolder, std::memory_order::relaxed);
		}

		auto GetNetworkConnectivity() -> NetworkConnectivity {

			return infoHolder.load(std::memory_order::relaxed).Info.Connectivity;
		}

		auto NetworkConnectivityChanged(NetworkConnectivityChangedEventHandler const& handler) -> winrt::event_token {

			return networkConnectivityChangedEvent.add(handler);
		}

		auto NetworkConnectivityChanged(winrt::event_token token) -> void {

			networkConnectivityChangedEvent.remove(token);
		}

		auto GetNetworkConnectionCost() -> NetworkConnectionCost {

			return infoHolder.load(std::memory_order::relaxed).Info.ConnectionCost;
		}

		auto NetworkConnectionCostChanged(NetworkConnectionCostChangedEventHandler const& handler) -> winrt::event_token {

			return networkConnectionCostChangedEvent.add(handler);
		}

		auto NetworkConnectionCostChanged(winrt::event_token token) -> void {

			networkConnectionCostChangedEvent.remove(token);
		}

	private:

		auto GetConnectionProfile() -> winrt::ConnectionProfile try {

			return winrt::NetworkInformation::GetInternetConnectionProfile();
		}
		catch (winrt::hresult_error const&) {

			return nullptr;
		}

		auto OnNetworkStatusChanged(winrt::IInspectable const&) -> void {

			auto connectionProfile = GetConnectionProfile();

			auto newInfoHolder = InfoHolder{ .HasInfo = true, .Info = Info::FromConnectionProfile(connectionProfile) };
			auto oldInfoHolder = infoHolder.exchange(newInfoHolder, std::memory_order::relaxed);

			if (!oldInfoHolder.HasInfo)
				return;

			auto& oldInfo = oldInfoHolder.Info;
			auto& newInfo = newInfoHolder.Info;

			if (oldInfo.Connectivity != newInfo.Connectivity)
				networkConnectivityChangedEvent(newInfo.Connectivity);

			if (oldInfo.ConnectionCost != newInfo.ConnectionCost)
				networkConnectionCostChangedEvent(newInfo.ConnectionCost);
		}

		struct Info {

			static auto FromConnectionProfile(winrt::ConnectionProfile const& connectionProfile) -> Info try {

				if (!connectionProfile)
					return {};

				auto connectivityLevel = connectionProfile.GetNetworkConnectivityLevel();
				auto connectionCost = connectionProfile.GetConnectionCost();

				return {

					.Connectivity = static_cast<NetworkConnectivity>(connectivityLevel),
					.ConnectionCost = {
						.ApproachingDataLimit = connectionCost.ApproachingDataLimit(),
						.BackgroundDataUsageRestricted = connectionCost.BackgroundDataUsageRestricted(),
						.NetworkCostType = static_cast<NetworkCostType>(connectionCost.NetworkCostType()),
						.OverDataLimit = connectionCost.OverDataLimit(),
						.Roaming = connectionCost.Roaming()
					}
				};
			}
			catch (winrt::hresult_error const&) {

				return {};
			}

			NetworkConnectivity Connectivity{};
			NetworkConnectionCost ConnectionCost;
		};

		struct alignas(8) InfoHolder {

			bool HasInfo;
			Info Info;
		};

		std::atomic<InfoHolder> infoHolder;
		winrt::event<NetworkConnectivityChangedEventHandler> networkConnectivityChangedEvent;
		winrt::event<NetworkConnectionCostChangedEventHandler> networkConnectionCostChangedEvent;
		winrt::event_token networkStatusChangedToken;
	};

	auto networkInfoServiceInternal = NetworkInfoServiceInternal{};
}

namespace Citrine {

	auto NetworkInfoService::GetNetworkConnectivity() -> NetworkConnectivity {

		return networkInfoServiceInternal.GetNetworkConnectivity();
	}

	auto NetworkInfoService::NetworkConnectivityChanged(NetworkConnectivityChangedEventHandler const& handler) -> winrt::event_token {

		return networkInfoServiceInternal.NetworkConnectivityChanged(handler);
	}

	auto NetworkInfoService::NetworkConnectivityChanged(winrt::event_token token) -> void {

		networkInfoServiceInternal.NetworkConnectivityChanged(token);
	}

	auto NetworkInfoService::GetNetworkConnectionCost() -> NetworkConnectionCost {

		return networkInfoServiceInternal.GetNetworkConnectionCost();
	}

	auto NetworkInfoService::NetworkConnectionCostChanged(NetworkConnectionCostChangedEventHandler const& handler) -> winrt::event_token {

		return networkInfoServiceInternal.NetworkConnectionCostChanged(handler);
	}

	auto NetworkInfoService::NetworkConnectionCostChanged(winrt::event_token token) -> void {

		networkInfoServiceInternal.NetworkConnectionCostChanged(token);
	}
}