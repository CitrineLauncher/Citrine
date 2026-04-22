#include "pch.h"
#include "DataProtection.h"

#include <ncrypt.h>
#include <ncryptprotect.h>

namespace {

	class ProtectionDescriptor {
	public:

		using HandleType = ::NCRYPT_DESCRIPTOR_HANDLE;

		static auto CurrentUser() noexcept -> ProtectionDescriptor const& {

			static auto descriptor = ProtectionDescriptor{ L"LOCAL=user" };
			return descriptor;
		}

		static auto LocalMachine() noexcept -> ProtectionDescriptor const& {

			static auto descriptor = ProtectionDescriptor{ L"LOCAL=machine" };
			return descriptor;
		}

		operator HandleType() const noexcept {

			return handle;
		}

		~ProtectionDescriptor() noexcept {
		
			::NCryptCloseProtectionDescriptor(handle);
		}

	private:

		ProtectionDescriptor(wchar_t const* descriptorStr) noexcept {

			::NCryptCreateProtectionDescriptor(descriptorStr, 0, &handle);
		}

		ProtectionDescriptor(ProtectionDescriptor const&) = delete;
		auto operator=(ProtectionDescriptor const&) = delete;

		HandleType handle{ nullptr };
	};
}

namespace Citrine {

	auto DataProtection::ProtectData(std::span<std::uint8_t const> data, DataProtectionScope scope) noexcept -> ProtectDataResult {

		using enum DataProtectionScope;
		auto descriptor = ::NCRYPT_DESCRIPTOR_HANDLE{};
		switch (scope) {
		case CurrentUser:	descriptor = ProtectionDescriptor::CurrentUser();	break;
		case LocalMachine:	descriptor = ProtectionDescriptor::LocalMachine();	break;
		}

		if (!descriptor)
			return std::unexpected{ NTE_INVALID_HANDLE };

		auto result = ProtectDataResult{ std::in_place };
		auto status = ::NCryptProtectSecret(
			descriptor,
			0,
			data.data(),
			static_cast<::ULONG>(data.size()),
			nullptr,
			nullptr,
			result->put(),
			result->size_address<::ULONG>()
		);

		if (status != ERROR_SUCCESS)
			result = std::unexpected{ status };
		return result;
	}

	auto DataProtection::UnprotectData(std::span<std::uint8_t const> data) noexcept -> UnprotectDataResult {

		auto result = UnprotectDataResult{ std::in_place };
		auto status = ::NCryptUnprotectSecret(
			nullptr,
			0,
			data.data(),
			static_cast<::ULONG>(data.size()),
			nullptr,
			nullptr,
			result->put(),
			result->size_address<::ULONG>()
		);

		if (status != ERROR_SUCCESS)
			result = std::unexpected{ status };
		return result;
	}
}