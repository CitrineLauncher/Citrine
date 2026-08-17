#include "pch.h"
#include "FE3Handler.h"

#include "Generated Files/WUProtocolTemplates.h"

#include "Core/Util/Guid.h"
#include "Core/Util/DateTime.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Util/Ascii.h"
#include "Core/Codec/Base64.h"
#include "Services/HttpService.h"
#include "Core/Logging/Logger.h"

#include <format>
#include <algorithm>

#include <pugixml.hpp>

using namespace Citrine;
using namespace Windows;

namespace {

	constexpr auto& RequestUrl = "https://fe3.delivery.mp.microsoft.com/ClientWebService/client.asmx/secured";

	class FE3HandlerInternal {
	public:

		static auto GetCookieAsync() -> AsyncFE3Result<FE3Cookie> {

			auto result = co_await InvokeGetCookie().ResumeAgile();
			if (!result) {

				Logger::Error("Fetching cookie failed: {}", result.error());
			}
			co_return result;
		}

		static auto GetUpdatesAsync(Guid categoryId, FE3Cookie const& cookie) -> AsyncFE3Result<std::vector<FE3UpdateInfo>> {

			auto result = co_await InvokeGetUpdates(categoryId, cookie).ResumeAgile();
			if (!result) {

				Logger::Error("Fetching updates for category {} failed: {}", categoryId, result.error());
			}
			co_return result;
		}

		static auto GetFileUrlAsync(Guid updateId, int revisionNumber) -> AsyncFE3Result<Url> {

			auto result = co_await InvokeGetFileUrl(updateId, revisionNumber).ResumeAgile();
			if (!result) {

				Logger::Error("Fetching file url for update (UpdateId: {}, RevisionNumber: {}) failed: {}", updateId, revisionNumber, result.error());
			}
			co_return result;
		}

	private:

		static auto InvokeGetCookie() -> AsyncFE3Result<FE3Cookie> {

			auto buildPayload = [] static {

				auto messageId = Guid::Create();
				auto created = DateTime::Now();
				auto expires = created + 5min;

				return std::format(WUProtocolTemplates::GetCookie, messageId, RequestUrl, created, expires);
			};

			auto content = HttpContent{};
			content.Payload = buildPayload();
			content.Headers.Insert("Content-Type", "application/soap+xml; charset=utf-8");

			auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, RequestUrl, std::move(content)).ResumeAgile();
			if (!responseMessage)
				co_return FE3Error::NetworkError;

			if (!responseMessage->IsSuccessful())
				co_return FE3Error::ApiError;

			auto& responseContent = responseMessage->Content;
			auto response = pugi::xml_document{};
			if (!response.load_buffer_inplace(responseContent.data(), responseContent.size(), pugi::parse_default, pugi::encoding_utf8))
				co_return FE3Error::ResponseError;

			auto cookie = FE3Cookie{};

			auto result = response.select_node("/s:Envelope/s:Body/GetCookieResponse/GetCookieResult").node();
			auto expirationElement = result.child("Expiration");
			auto encryptedDataElement = result.child("EncryptedData");

			if (!DateTime::Parse(expirationElement.text().as_string(), cookie.Expiration))
				co_return FE3Error::ResponseError;

			cookie.EncryptedData = encryptedDataElement.text().as_string();
			if (cookie.EncryptedData.empty() || !Base64::Validate(cookie.EncryptedData))
				co_return FE3Error::ResponseError;

			co_return cookie;
		}

		static auto InvokeGetUpdates(Guid const& categoryId, FE3Cookie const& cookie) -> AsyncFE3Result<std::vector<FE3UpdateInfo>> {

			auto buildPayload = [&] {

				auto messageId = Guid::Create();
				auto created = DateTime::Now();
				auto expires = created + 5min;

				return std::format(WUProtocolTemplates::SyncUpdates, messageId, RequestUrl, created, expires, cookie.Expiration, cookie.EncryptedData, categoryId);
			};

			auto content = HttpContent{};
			content.Payload = buildPayload();
			content.Headers.Insert("Content-Type", "application/soap+xml; charset=utf-8");

			auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, RequestUrl, std::move(content)).ResumeAgile();
			if (!responseMessage)
				co_return FE3Error::NetworkError;

			if (!responseMessage->IsSuccessful())
				co_return FE3Error::ApiError;

			auto& responseContent = responseMessage->Content;
			auto response = pugi::xml_document{};
			if (!response.load_buffer_inplace(responseContent.data(), responseContent.size(), pugi::parse_default, pugi::encoding_utf8))
				co_return FE3Error::ResponseError;

			auto updates = std::vector<FE3UpdateInfo>{};
			updates.reserve(8);

			auto result = response.select_node("/s:Envelope/s:Body/SyncUpdatesResponse/SyncUpdatesResult").node();
			for (auto node : result.select_nodes("NewUpdates/UpdateInfo/Xml")) {

				auto fragmentBuffer = std::string{ node.node().text().get() };
				auto fragment = pugi::xml_document{};
				if (!fragment.load_buffer_inplace(fragmentBuffer.data(), fragmentBuffer.size(), pugi::parse_fragment, pugi::encoding_utf8))
					co_return FE3Error::ResponseError;

				auto updateIdentityElement = fragment.child("UpdateIdentity");
				auto propertiesElement = fragment.child("Properties");
				auto appxMetadataElement = fragment.select_node("ApplicabilityRules/Metadata/AppxPackageMetadata/AppxMetadata").node();

				if (!updateIdentityElement || !appxMetadataElement)
					continue;

				auto& update = updates.emplace_back();
				auto& [updateIdentity, packageMetadata] = update;

				auto updateIdAttribute = updateIdentityElement.attribute("UpdateID");
				auto revisionNumberAttribute = updateIdentityElement.attribute("RevisionNumber");

				if (!Guid::Parse(updateIdAttribute.as_string(), updateIdentity.UpdateId))
					co_return FE3Error::ResponseError;

				if (!ParseInteger(revisionNumberAttribute.as_string(), updateIdentity.RevisionNumber))
					co_return FE3Error::ResponseError;

				auto isAppxFrameworkAttribute = propertiesElement.attribute("IsAppxFramework");

				packageMetadata.IsFramework = isAppxFrameworkAttribute.as_bool();

				auto packageMonikerAttribute = appxMetadataElement.attribute("PackageMoniker");
				auto isAppxBundleAttribute = appxMetadataElement.attribute("IsAppxBundle");

				packageMetadata.PackageId = packageMonikerAttribute.as_string();
				if (!packageMetadata.PackageId.IsValid())
					co_return FE3Error::ResponseError;

				packageMetadata.IsBundle = isAppxBundleAttribute.as_bool();

				auto it = std::ranges::find_if(updates.begin(), updates.end() - 1, [&update](FE3UpdateInfo const& otherUpdate) {

					auto& fullName = update.PackageMetadata.PackageId.FullName();
					auto& otherFullName = otherUpdate.PackageMetadata.PackageId.FullName();

					return Ascii::CaseInsensitiveEquals(fullName, otherFullName);
				});

				if (std::to_address(it) != &update) {

					updates.pop_back();
				}
			}

			co_return updates;
		}

		static auto InvokeGetFileUrl(Guid const& updateId, int revisionNumber) -> AsyncFE3Result<Url> {

			auto buildPayload = [&] {

				auto messageId = Guid::Create();
				auto created = DateTime::Now();
				auto expires = created + 5min;

				return std::format(WUProtocolTemplates::GetExtendedUpdateInfo2, messageId, RequestUrl, created, expires, updateId, revisionNumber);
			};

			auto content = HttpContent{};
			content.Payload = buildPayload();
			content.Headers.Insert("Content-Type", "application/soap+xml; charset=utf-8");

			auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, RequestUrl, std::move(content)).ResumeAgile();
			if (!responseMessage)
				co_return FE3Error::NetworkError;

			if (!responseMessage->IsSuccessful())
				co_return FE3Error::ApiError;

			auto& responseContent = responseMessage->Content;
			auto response = pugi::xml_document{};
			if (!response.load_buffer_inplace(responseContent.data(), responseContent.size(), pugi::parse_default, pugi::encoding_utf8))
				co_return FE3Error::ResponseError;

			auto result = response.select_node("/s:Envelope/s:Body/GetExtendedUpdateInfo2Response/GetExtendedUpdateInfo2Result").node();
			for (auto node : result.select_nodes("FileLocations/FileLocation/Url")) {

				auto urlView = UrlView{ node.node().text().as_string() };
				if (urlView.Host() == "tlu.dl.delivery.mp.microsoft.com")
					co_return Url{ urlView };
			}

			co_return FE3Error::ContentNotFound;
		}
	};
}

namespace Citrine::Windows {

	auto FE3Cookie::IsValid() const noexcept -> bool {

		return (DateTime::Now() + 5min) < Expiration && !EncryptedData.empty();
	}

	auto FE3Handler::GetCookieAsync() -> AsyncFE3Result<FE3Cookie> {

		return FE3HandlerInternal::GetCookieAsync();
	}

	auto FE3Handler::GetUpdatesAsync(Guid const& categoryId, FE3Cookie const& cookie) -> AsyncFE3Result<std::vector<FE3UpdateInfo>> {

		return FE3HandlerInternal::GetUpdatesAsync(categoryId, cookie);
	}

	auto FE3Handler::GetFileUrlAsync(FE3UpdateIdentity const& updateIdentity) -> AsyncFE3Result<Url> {

		return FE3HandlerInternal::GetFileUrlAsync(updateIdentity.UpdateId, updateIdentity.RevisionNumber);
	}

	auto FE3Handler::GetFileUrlAsync(Guid const& updateId, int revisionNumber) -> AsyncFE3Result<Url> {

		return FE3HandlerInternal::GetFileUrlAsync(updateId, revisionNumber);
	}
}