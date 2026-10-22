#pragma once

#include "Core/Util/Concepts.h"

#include <compare>
#include <format>

namespace Citrine {

	enum struct StorageError {

		None,
		NotOpen,
		NotFound,
		OpeningFailed,
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

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::StorageError, CharT> : formatter<int, CharT> {

		auto format(::Citrine::StorageError value, auto& ctx) const -> auto {

			return formatter<int, CharT>::format(std::to_underlying(value), ctx);
		}
	};
}