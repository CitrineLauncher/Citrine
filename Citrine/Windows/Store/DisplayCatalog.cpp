#include "pch.h"
#include "DisplayCatalog.h"

#include "Core/Net/Url.h"
#include "Core/Net/UrlQuery.h"
#include "Services/HttpService.h"
#include "Core/Logging/Logger.h"

namespace Citrine::Windows::Store {

	auto DisplayCatalog::GetProductIdFromPackageFamilyNameAsync(std::string packageFamilyName) -> AsyncDisplayCatalogResult<std::string> {

		auto query = UrlQuery{

			{ "market", "neutral" },
			{ "languages", "neutral" },
			{ "fieldsTemplate", "empty" },
			{ "alternateid", "PackageFamilyName" },
			{ "value", packageFamilyName }
		};

		auto rawUrl = std::string{ "https://displaycatalog.mp.microsoft.com/v7.0/products/lookup" };
		rawUrl.push_back('?');
		query.Serialize(AppendTo(rawUrl));

		auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, std::move(rawUrl)).ResumeAgile();
		if (!responseMessage) {

			Logger::Error("Fetching product id for package family {} failed: network error", packageFamilyName);
			co_return DisplayCatalogError::NetworkError;
		}

		if (!responseMessage->IsSuccessful()) {

			Logger::Error("Fetching product id for package family {} failed: api error", packageFamilyName);
			co_return DisplayCatalogError::ApiError;
		}

		auto response = DisplayCatalogProductsResponse{};

		constexpr auto opts = glz::opts{ .null_terminated = false, .error_on_unknown_keys = false };
		if (auto ec = glz::read<opts>(response, responseMessage->Content); ec) {

			Logger::Error("Fetching product id for package family {} failed: response error", packageFamilyName);
			co_return DisplayCatalogError::ResponseError;
		}

		if (response.ProductIds.empty()) {

			Logger::Error("Fetching product id for package family {} failed: product not found", packageFamilyName);
			co_return DisplayCatalogError::ProductNotFound;
		}

		co_return response.ProductIds.front();
	}
}