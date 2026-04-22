#include "pch.h"
#include "JsonStorage.h"

namespace Citrine {

	JsonStorageBase::JsonStorageBase(std::filesystem::path const& path) {

		Open(path);
	}

	auto JsonStorageBase::Open(std::filesystem::path const& path) -> bool {

		return file.Open(path, FileMode::OpenAlways, FileAccess::ReadWrite);
	}

	auto JsonStorageBase::IsOpen() const noexcept -> bool {

		return file.IsOpen();
	}

	JsonStorageBase::operator bool() const noexcept {

		return IsOpen();
	}

	auto JsonStorageBase::Close() noexcept -> void {

		file.Close();
	}
}