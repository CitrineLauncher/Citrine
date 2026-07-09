#pragma once

#include "winrt/Citrine.h"

#include <concepts>

#include <winrt/Windows.UI.Xaml.Interop.h>
#include <unknwnbase.h>

namespace Citrine {

	struct __declspec(uuid("bbefef2e-be92-4ef1-8674-c0418d84d914")) IItemFilterInterop : public ::IUnknown {

		virtual auto Match(void const* item) const -> bool = 0;
	};

	template<typename T, std::invocable<T const&> Predicate>
	class ItemFilter : public winrt::implements<ItemFilter<T, Predicate>, winrt::Citrine::IItemFilter, IItemFilterInterop> {
	public:

		template<typename P>
		ItemFilter(P&& predicate)

			: predicate(std::forward<P>(predicate))
		{}

		auto Match(winrt::Windows::Foundation::IInspectable const& item) const -> bool {

			return predicate(item.as<T>());
		}

		auto Match(void const* item) const -> bool final override {

			return predicate(*static_cast<T const*>(item));
		}

		auto ItemType() const -> winrt::Windows::UI::Xaml::Interop::TypeName {

			return winrt::xaml_typename<T>();
		}

	private:

		Predicate predicate;
	};

	template<typename T, std::invocable<T const&> P>
	auto MakeItemFilter(P&& predicate) -> winrt::Citrine::IItemFilter {

		return winrt::make<ItemFilter<T, std::remove_cvref_t<P>>>(std::forward<P>(predicate));
	}

	template<typename T>
	auto EmptyItemFilter() -> winrt::Citrine::IItemFilter {

		static auto filter = MakeItemFilter<T>([](T const&) static { return true; });
		return filter;
	}

	template<typename C>
	auto GetItemFilterMatchCount(C const& collection, winrt::Citrine::IItemFilter const& filter) -> std::size_t {

		auto matchCount = 0uz;

		if constexpr (requires{ std::begin(collection); std::end(collection); }) {

			auto it = std::begin(collection);
			auto end = std::end(collection);

			using T = std::remove_cvref_t<decltype(*it)>;
			if (filter.ItemType() != winrt::xaml_typename<T>())
				throw winrt::hresult_invalid_argument{};

			if constexpr (!std::same_as<T, winrt::Windows::Foundation::IInspectable>) {

				auto filterInterop = filter.as<IItemFilterInterop>();
				while (it != end) {

					decltype(auto) item = *it;
					matchCount += static_cast<std::size_t>(filterInterop->Match(&item));
					++it;
				}
			}
			else {

				while (it != end) {

					decltype(auto) item = *it;
					matchCount += static_cast<std::size_t>(filter.Match(item));
					++it;
				}
			}
		}
		else {

			auto it = collection.First();
			while (it.HasCurrent()) {

				matchCount += static_cast<std::size_t>(filter.Match(it.Current()));
				it.MoveNext();
			}
		}

		return matchCount;
	}
}