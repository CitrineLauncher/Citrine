#include "pch.h"
#include "KeyRegistry.h"

#include "Core/Util/Frozen.h"

#include <array>

#include "Generated Files/XboxCikEntries.h"

using namespace Citrine;
using namespace Xbox;

namespace Citrine::Xbox {

	auto KeyRegistry::GetCik(Guid id) noexcept -> CikEntry const* {

		struct EntryComparer : decltype([](auto const& left, auto const& right) static -> bool {

			constexpr auto getId = []<typename T>(T const& obj) -> Guid const& {

				if constexpr (std::same_as<T, CikEntry>)
					return obj.Id;
				else
					return obj;
			};
			return getId(left) < getId(right);
		})
		{ 
			using is_transparent = void;
		};

		static constexpr auto cikEntries = MakeFrozenSet<CikEntry, EntryComparer>(GetCikEntries());

		auto it = cikEntries.find(id);
		return it != cikEntries.end() ? std::to_address(it) : nullptr;
	}
}