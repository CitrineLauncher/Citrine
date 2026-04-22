#pragma once

#include "CikEntry.h"

#include "Core/Util/Guid.h"

namespace Citrine::Xbox {

	class KeyRegistry {
	public:

		static auto GetCik(Guid id) noexcept -> CikEntry const*;
	};
}