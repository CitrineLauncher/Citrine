#pragma once

#include "Common.h"

#include "Core/IO/File.h"

#include <string>
#include <concepts>
#include <functional>

#include <glaze/json.hpp>

namespace Citrine {

	class JsonStorageBase {
	public:

		using OperationCallback = std::move_only_function<auto(StorageOperationResult, std::string const&) -> void>;

		auto Open(std::filesystem::path const& path) -> bool;
		auto IsOpen() const noexcept -> bool;
		explicit operator bool() const noexcept;
		auto Close() noexcept -> void;

	protected:

		JsonStorageBase() noexcept = default;
		explicit JsonStorageBase(std::filesystem::path const& path);

		JsonStorageBase(JsonStorageBase const&) = delete;
		auto operator=(JsonStorageBase const&) = delete;

		File file;
		std::string buffer;
	};

	template<std::default_initializable T>
	class JsonStorage : public JsonStorageBase {
	public:

		using JsonStorageBase::JsonStorageBase;

		auto Load() -> StorageOperationResult {

			if (!file)
				return { StorageError::NotOpen };

			file.SeekToBegin();
			if (!file.ReadToEnd(buffer))
				return { StorageError::ReadingFailed };

			if (auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false }>(obj, buffer))
				return { StorageError::DeserializationFailed };

			return {};
		}

		auto Load(OperationCallback callback) -> StorageOperationResult {

			auto result = Load();
			callback(result, buffer);
			return result;
		}

		auto Save() -> StorageOperationResult {

			if (!file)
				return { StorageError::NotOpen };

			if (auto ec = glz::write<glz::opts{ .prettify = true }>(obj, buffer))
				return { StorageError::SerializationFailed };

			file.SeekToBegin();
			if (!file.Write(buffer) || !file.Truncate())
				return { StorageError::WritingFailed };

			return {};
		}

		auto Save(OperationCallback callback) -> StorageOperationResult {

			auto result = Save();
			callback(result, buffer);
			return result;
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
