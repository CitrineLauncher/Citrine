#pragma once

#include <compare>

namespace Citrine {

	enum struct StorageError {

		None,
		NotOpen,
		ReadingFailed,
		WritingFailed,
		SerializationFailed,
		DeserializationFailed,
		EncryptionFailed,
		DecryptionFailed
	};

	struct StorageOperationResult {

		explicit constexpr operator bool() const noexcept {

			return Error == StorageError::None;
		}

		constexpr auto operator==(StorageError value) noexcept -> bool {

			return Error == value;
		}

		constexpr auto operator<=>(StorageError value) noexcept -> std::strong_ordering {

			return Error <=> value;
		}

		StorageError Error{};
	};
}