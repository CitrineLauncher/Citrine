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

			decltype(auto) obj = [&] -> decltype(auto) {

				if constexpr (std::same_as<T, winrt::Windows::Foundation::IInspectable>) {

					return (item);
				}
				else {

					return item.as<T>();
				}
			}();

			return predicate(obj);
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
}