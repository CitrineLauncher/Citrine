#include "pch.h"
#include "ProtectedJsonStorage.h"

namespace Citrine {

	ProtectedJsonStorageBase::ProtectedJsonStorageBase(std::filesystem::path const& path, DataProtectionScope scope) {

		Open(path, scope);
	}

	auto ProtectedJsonStorageBase::Open(std::filesystem::path const& path, DataProtectionScope scope) -> bool {

		auto result = file.Open(path, FileMode::OpenAlways, FileAccess::ReadWrite);
		if (result) protectionScope = scope;
		return result;
	}

	auto ProtectedJsonStorageBase::IsOpen() const noexcept -> bool {

		return file.IsOpen();
	}

	ProtectedJsonStorageBase::operator bool() const noexcept {

		return IsOpen();
	}

	auto ProtectedJsonStorageBase::Close() noexcept -> void {

		file.Close();
	}
}