#pragma once

#include <cstdint>
#include <concepts>
#include <compare>

namespace Citrine::Xbox {

	struct XvcRegionId {

		static const XvcRegionId Invalid;
		static const XvcRegionId XvcMetadata;
		static const XvcRegionId XvcFileSystem;
		static const XvcRegionId RegistrationFiles;
		static const XvcRegionId EmbeddedXvd;
		static const XvcRegionId Header;
		static const XvcRegionId MutableXvcMetadata;

		constexpr auto IsValid() const noexcept -> bool;
		constexpr auto IsSystem() const noexcept -> bool;

		constexpr operator auto() const noexcept {

			enum struct EnumType : std::uint32_t {};
			return static_cast<EnumType>(Value);
		}

		template<std::integral T>
		explicit constexpr operator T() const noexcept {

			return static_cast<T>(Value);
		}

		constexpr auto operator<=>(XvcRegionId const&) const noexcept -> std::strong_ordering = default;

		std::uint32_t Value;
	};

	inline constexpr XvcRegionId XvcRegionId::Invalid				{ 0x00000000 };
	inline constexpr XvcRegionId XvcRegionId::XvcMetadata			{ 0x40000001 };
	inline constexpr XvcRegionId XvcRegionId::XvcFileSystem			{ 0x40000002 };
	inline constexpr XvcRegionId XvcRegionId::RegistrationFiles		{ 0x40000003 };
	inline constexpr XvcRegionId XvcRegionId::EmbeddedXvd			{ 0x40000004 };
	inline constexpr XvcRegionId XvcRegionId::Header				{ 0x40000005 };
	inline constexpr XvcRegionId XvcRegionId::MutableXvcMetadata	{ 0x40000006 };

	inline constexpr auto XvcRegionId::IsValid() const noexcept -> bool {

		return Value > 0;
	}

	inline constexpr auto XvcRegionId::IsSystem() const noexcept -> bool {

		return Value >= 0x3FFFFFFF;
	}
}