#include "pch.h"
#include "FE3Handler.h"

#include "Core/Util/Guid.h"
#include "Core/Util/DateTime.h"
#include "Services/HttpService.h"
#include "Core/Logging/Logger.h"

#include <format>

#include <pugixml.hpp>

namespace {

	constexpr auto& GetFileUrlTemplate{
R"(<s:Envelope xmlns:a="http://www.w3.org/2005/08/addressing" xmlns:s="http://www.w3.org/2003/05/soap-envelope">
	<s:Header>
		<a:Action s:mustUnderstand="1">http://www.microsoft.com/SoftwareDistribution/Server/ClientWebService/GetExtendedUpdateInfo2</a:Action>
		<a:MessageID>urn:uuid:{0}</a:MessageID>
		<a:To s:mustUnderstand="1">https://fe3.delivery.mp.microsoft.com/ClientWebService/client.asmx/secured</a:To>
		<o:Security s:mustUnderstand="1" xmlns:o="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd">
			<Timestamp xmlns="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd">
				<Created>{1}</Created>
				<Expires>{2}</Expires>
			</Timestamp>
			<wuws:WindowsUpdateTicketsToken wsu:id="ClientMSA" xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd" xmlns:wuws="http://schemas.microsoft.com/msus/2014/10/WindowsUpdateAuthorization">
				<TicketType Name="ADD" Version="1.0" Policy="MBI_SSL" />
			</wuws:WindowsUpdateTicketsToken>
		</o:Security>
	</s:Header>
	<s:Body>
		<GetExtendedUpdateInfo2 xmlns="http://www.microsoft.com/SoftwareDistribution/Server/ClientWebService">
			<updateIDs>
				<UpdateIdentity>
					<UpdateID>{3}</UpdateID>
					<RevisionNumber>{4}</RevisionNumber>
				</UpdateIdentity>
			</updateIDs>
			<infoTypes>
				<XmlUpdateFragmentType>FileUrl</XmlUpdateFragmentType>
				<XmlUpdateFragmentType>FileDecryption</XmlUpdateFragmentType>
				<XmlUpdateFragmentType>EsrpDecryptionInformation</XmlUpdateFragmentType>
				<XmlUpdateFragmentType>PiecesHashUrl</XmlUpdateFragmentType>
				<XmlUpdateFragmentType>BlockMapUrl</XmlUpdateFragmentType>
			</infoTypes>
			<deviceAttributes>FlightRing=Retail;IsFlightingEnabled=0;DeviceFamily=Windows.Desktop;</deviceAttributes>
		</GetExtendedUpdateInfo2>
	</s:Body>
</s:Envelope>)"
	};
}

namespace Citrine::Windows {

	auto FE3Handler::GetFileUrlAsync(std::string updateId, std::uint64_t revisionNumber) -> AsyncFE3Result<Url> {

		auto buildPayload = [&] {

			auto messageId = Guid::Create();
			auto created = DateTime::Now();
			auto expires = created + 5min;

			return std::format(GetFileUrlTemplate, messageId, created, expires, updateId, revisionNumber);
		};

		auto content = HttpContent{};
		content.Payload = buildPayload();
		content.Headers.Insert("Content-Type", "application/soap+xml; charset=utf-8");

		auto responseMessage = co_await HttpService::SendRequestAsync(HttpMethod::Get, "https://fe3.delivery.mp.microsoft.com/ClientWebService/client.asmx/secured", std::move(content)).ResumeAgile();
		if (!responseMessage) {

			Logger::Error("Fetching FE3FileUrl for update id {} failed: network error", updateId);
			co_return FE3Error::NetworkError;
		}

		if (!responseMessage->IsSuccessful()) {

			Logger::Error("Fetching FE3FileUrl for update id {} failed: api error", updateId);
			co_return FE3Error::ApiError;
		}

		auto& responseContent = responseMessage->Content;
		auto response = pugi::xml_document{};
		if (!response.load_buffer_inplace(responseContent.data(), responseContent.size(), pugi::parse_default, pugi::encoding_utf8)) {

			Logger::Error("Fetching FE3FileUrl for update id {} failed: response error", updateId);
			co_return FE3Error::ResponseError;
		}

		auto result = response.select_node("/s:Envelope/s:Body/GetExtendedUpdateInfo2Response/GetExtendedUpdateInfo2Result");
		for (auto node : result.node().select_nodes("FileLocations/FileLocation/Url")) {

			auto urlView = UrlView{ node.node().text().as_string() };
			if (urlView.Host() == "tlu.dl.delivery.mp.microsoft.com") {

				co_return Url{ urlView };
			}
		}

		Logger::Error("Fetching FE3FileUrl for update id {} failed: content not found", updateId);
		co_return FE3Error::ContentNotFound;
	}
}