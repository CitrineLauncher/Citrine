#pragma once

#include "GamePackage.h"
#include "PackageAction.h"
#include "PackageOperation.h"
#include "PackageOperationCollection.h"

#include "Core/Coroutine/Task.h"
#include "Core/Util/JsonWrappers.h"

#include <filesystem>
#include <variant>

#include <glaze/json.hpp>

namespace Citrine::Minecraft::Bedrock {

	struct GamePackageOperation {

		std::variant<GamePackageIdentity, GamePackage> Package;
		std::filesystem::path PackageLocation;
		PackageAction Action{};
		mutable PackageOperationStatus Status{};
		mutable Task<bool> Task;

		auto IsPersistent() const noexcept -> bool {

			using enum PackageAction;
			using enum PackageOperationStatus;

			return
				(Action == Install || Action == Import || Action == Uninstall) &&
				(Status != Completed && Status != Cancelled);
		}
	};

	struct GamePackageOperationEqualityComparer {

		using is_transparent = void;

		static auto operator()(auto const& left, auto const& right) noexcept -> bool {

			constexpr auto packageEqual = GamePackageEqualityComparer{};
			auto leftPackageId = GetPackageId(left);
			auto rightPackageId = GetPackageId(right);

			if (!leftPackageId || !rightPackageId)
				return !leftPackageId && !rightPackageId;
			return packageEqual(*leftPackageId, *rightPackageId);
		}

		static auto GetPackageId(GamePackageOperation const& op) noexcept -> GamePackageIdentity const* {

			auto id = std::get_if<GamePackageIdentity>(&op.Package);
			if (!id)
				id = std::get_if<GamePackage>(&op.Package);
			return id;
		}

		static auto GetPackageId(std::derived_from<GamePackageIdentity> auto const& package) noexcept -> GamePackageIdentity const* {

			return &package;
		}
	};

	using GamePackageOperationCollection = PackageOperationCollection<GamePackageOperation, GamePackageOperationEqualityComparer>;
}

namespace glz {

	template<>
	struct meta<::Citrine::Minecraft::Bedrock::GamePackageOperation> {

		using T = ::Citrine::Minecraft::Bedrock::GamePackageOperation;

		static constexpr auto read_CancellationRequested = [](T& self, bool value) static noexcept -> void {

			using enum ::Citrine::Minecraft::Bedrock::PackageOperationStatus;

			if (value)
				self.Status = CancellationRequested;
		};

		static constexpr auto write_CancellationRequested = [](T const& self) static noexcept -> bool {

			using enum ::Citrine::Minecraft::Bedrock::PackageOperationStatus;

			return self.Status == CancellationRequested;
		};

		static constexpr auto value = object(
			"Package", &T::Package,
			"PackageLocation", SkipDefault<&T::PackageLocation>,
			"Action", &T::Action,
			"CancellationRequested", custom<read_CancellationRequested, SkipDefault<write_CancellationRequested>>
		);
	};
}