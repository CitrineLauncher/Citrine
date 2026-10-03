#include "pch.h"
#include "GameConfig.h"

#include "Core/IO/File.h"
#include "Core/Util/Scope.h"

using namespace Citrine;
using namespace Minecraft::Bedrock;

namespace {

	auto LoadGameConfig(std::filesystem::path const& path, GameConfig& gameConfig, std::string& buffer) -> StorageOperationResult {

		auto file = File{ path, FileMode::OpenExisting, FileAccess::Read, FileShare::Read | FileShare::Delete };
		if (!file) {

			return { file.LastError() == ERROR_FILE_NOT_FOUND 
				? StorageError::NotFound
				: StorageError::OpeningFailed
			};
		}

		if (!file.ReadToEnd(buffer))
			return { StorageError::ReadingFailed };

		if (auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false }>(gameConfig, buffer); ec)
			return { StorageError::DeserializationFailed };

		return {};
	}

	auto SaveGameConfig(std::filesystem::path const& path, GameConfig const& gameConfig, std::string& buffer) -> StorageOperationResult {

		auto tempFile = File{ path.native() + L".temp", FileMode::OpenAlways, FileAccess::Write | FileAccess::Delete, FileShare::None };
		if (!tempFile)
			return { StorageError::OpeningFailed };

		auto deleteTempFile = ScopeExit{ [&tempFile] { tempFile.Delete(); } };

		if (auto ec = glz::write<glz::opts{ .prettify = true }>(gameConfig, buffer); ec)
			return { StorageError::SerializationFailed };

		if (!tempFile.Write(buffer) || !tempFile.Truncate())
			return { StorageError::WritingFailed };

		if (!tempFile.Rename(path, true))
			return { StorageError::WritingFailed };

		deleteTempFile.Release();
		return {};
	}
}

namespace Citrine::Minecraft::Bedrock {

	auto GameConfig::Load(std::filesystem::path const& path) -> std::expected<GameConfig, StorageError> {

		auto gameConfig = std::expected<GameConfig, StorageError>{};

		auto buffer = std::string{};
		auto result = LoadGameConfig(path, *gameConfig, buffer);

		if (!result)
			gameConfig = std::unexpected{ result.Error };
		return gameConfig;
	}

	auto GameConfig::Load(std::filesystem::path const& path, OperationCallback callback) -> std::expected<GameConfig, StorageError> {

		auto gameConfig = std::expected<GameConfig, StorageError>{};

		auto buffer = std::string{};
		auto result = LoadGameConfig(path, *gameConfig, buffer);

		callback(result, buffer);

		if (!result)
			gameConfig = std::unexpected{ result.Error };
		return gameConfig;
	}

	auto GameConfig::Save(this GameConfig const& gameConfig, std::filesystem::path const& path) -> std::expected<void, StorageError> {

		auto buffer = std::string{};
		auto result = SaveGameConfig(path, gameConfig, buffer);

		if (!result)
			return std::unexpected{ result.Error };
		return {};
	}

	auto GameConfig::Save(this GameConfig const& gameConfig, std::filesystem::path const& path, OperationCallback callback) -> std::expected<void, StorageError> {

		auto buffer = std::string{};
		auto result = SaveGameConfig(path, gameConfig, buffer);

		callback(result, buffer);

		if (!result)
			return std::unexpected{ result.Error };
		return {};
	}
}