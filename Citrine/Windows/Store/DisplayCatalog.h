#pragma once

#include "Core/Coroutine/Task.h"

#include <string>
#include <expected>

#include <glaze/json.hpp>

namespace Citrine::Windows::Store {

	enum struct DisplayCatalogError {

		Unknown,
		NetworkError,
		ApiError,
		ResponseError,
		ProductNotFound
	};

	template<typename T = void>
	struct DisplayCatalogResult : std::expected<T, DisplayCatalogError> {

		using DisplayCatalogResult::expected::expected;

		DisplayCatalogResult(DisplayCatalogError error) noexcept

			: DisplayCatalogResult::expected(std::unexpect, error)
		{}
	};

	template<typename T = void>
	using AsyncDisplayCatalogResult = Task<DisplayCatalogResult<T>>;

	struct DisplayCatalogProductsResponse {

		std::vector<std::string> ProductIds;
		glz::generic Aggregations;
		bool HasMorePages{};
		glz::generic Products;
		std::size_t TotalResultCount{};
	};

	class DisplayCatalog {
	public:

		static auto GetProductIdFromPackageFamilyNameAsync(std::string packageFamilyName) -> AsyncDisplayCatalogResult<std::string>;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Windows::Store::DisplayCatalogProductsResponse> {

		using T = ::Citrine::Windows::Store::DisplayCatalogProductsResponse;

		static constexpr auto value = object(
			"BigIds", &T::ProductIds,
			"ProductIds", &T::ProductIds,
			"Aggregations", &T::Aggregations,
			"HasMorePages", &T::HasMorePages,
			"Products", &T::Products,
			"TotalResultCount", &T::TotalResultCount
		);
	};
}