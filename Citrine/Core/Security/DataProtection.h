#pragma once

#include <expected>
#include <string>
#include <span>

#include <wil/resource.h>

namespace Citrine {

	using ProtectDataResult = std::expected<wil::unique_hlocal_array_ptr<std::uint8_t>, std::int32_t>;
	using UnprotectDataResult = std::expected<wil::unique_hlocal_array_ptr<std::uint8_t>, std::int32_t>;

	enum struct DataProtectionScope {

		CurrentUser,
		LocalMachine
	};

	struct DataProtection {

		static auto ProtectData(std::span<std::uint8_t const> data, DataProtectionScope scope) noexcept -> ProtectDataResult;
		static auto UnprotectData(std::span<std::uint8_t const> data) noexcept -> UnprotectDataResult;
	};
}