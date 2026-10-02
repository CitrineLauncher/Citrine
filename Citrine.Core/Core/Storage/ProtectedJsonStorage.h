#pragma once

#include "Common.h"

#include "Core/IO/File.h"
#include "Core/Security/DataProtection.h"

#include <string>
#include <concepts>
#include <functional>

#include <glaze/json.hpp>

namespace Citrine {

	class ProtectedJsonStorageBase {
	public:

		auto Open(std::filesystem::path const& path, DataProtectionScope scope) -> bool;
		auto IsOpen() const noexcept -> bool;
		explicit operator bool() const noexcept;
		auto Close() noexcept -> void;

	protected:

		ProtectedJsonStorageBase() noexcept = default;
		ProtectedJsonStorageBase(std::filesystem::path const& path, DataProtectionScope scope);

		ProtectedJsonStorageBase(ProtectedJsonStorageBase const&) = delete;
		auto operator=(ProtectedJsonStorageBase const&) = delete;

		File file;
		DataProtectionScope protectionScope{};
	};

	template<std::default_initializable T>
	class ProtectedJsonStorage : public ProtectedJsonStorageBase {
	public:

		auto Load() -> StorageOperationResult {

			if (!file)
				return { StorageError::NotOpen };

			auto blob = std::string{};
			file.SeekToBegin();
			if (!file.ReadToEnd(blob))
				return { StorageError::ReadingFailed };

			auto json = DataProtection::UnprotectData({ reinterpret_cast<std::uint8_t const*>(blob.data()), blob.size() });
			if (!json)
				return { StorageError::DecryptionFailed };

			if (auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false, .null_terminated = false }>(obj, json))
				return { StorageError::DeserializationFailed };

			return {};
		}

		auto Save() -> StorageOperationResult {

			if (!file)
				return { StorageError::NotOpen };

			auto json = std::string{};
			if (auto ec = glz::write<glz::opts{}>(obj, json))
				return { StorageError::SerializationFailed };

			auto blob = DataProtection::ProtectData({ reinterpret_cast<std::uint8_t const*>(json.data()), json.size() }, protectionScope);
			if (!blob)
				return { StorageError::EncryptionFailed };

			file.SeekToBegin();
			if (!file.Write(blob) || !file.Truncate())
				return { StorageError::WritingFailed };

			return {};
		}

		auto Clear() -> StorageOperationResult {

			if (!file)
				return { StorageError::NotOpen };

			obj = {};

			file.SeekToBegin();
			if (!file.Truncate())
				return { StorageError::WritingFailed };

			return {};
		}

		template<typename Self>
		auto operator*(this Self&& self) noexcept -> decltype(auto) {

			return std::forward_like<Self&>(self.obj);
		}

		template<typename Self>
		auto operator->(this Self&& self) noexcept -> auto {

			return std::addressof(std::forward_like<Self&>(self.obj));
		}

	private:

		T obj{};
	};
}
