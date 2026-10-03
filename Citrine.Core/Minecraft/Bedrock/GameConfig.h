#pragma once

#include "Core/Storage/Common.h"

#include <filesystem>
#include <expected>
#include <functional>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	struct GameConfig {

		using OperationCallback = std::move_only_function<auto(StorageOperationResult, std::string const&) -> void>;

		static auto Load(std::filesystem::path const& path) -> std::expected<GameConfig, StorageError>;
		static auto Load(std::filesystem::path const& path, OperationCallback callback) -> std::expected<GameConfig, StorageError>;

		auto Save(this GameConfig const& gameConfig, std::filesystem::path const& path) -> std::expected<void, StorageError>;
		auto Save(this GameConfig const& gameConfig, std::filesystem::path const& path, OperationCallback callback) -> std::expected<void, StorageError>;

		bool ModLoaderEnabled{};
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GameConfig> {

		using T = ::Citrine::Minecraft::Bedrock::GameConfig;

		static constexpr auto value = object(
			"ModLoaderEnabled", &T::ModLoaderEnabled
		);
	};
}