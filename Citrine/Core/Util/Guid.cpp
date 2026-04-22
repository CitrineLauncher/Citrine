#include "pch.h"
#include "Guid.h"

#include <winrt/base.h>
#include <combaseapi.h>

namespace Citrine {

	auto Guid::Create() noexcept -> Guid {

		auto guid = ::GUID{};
		static_cast<void>(::CoCreateGuid(&guid));
		return std::bit_cast<Guid>(guid);
	}
}