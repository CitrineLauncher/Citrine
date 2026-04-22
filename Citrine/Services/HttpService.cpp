#include "pch.h"
#include "HttpService.h"

#include "Core/Logging/Logger.h"
#include "Core/IO/WinRTBuffer.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Util/FormatInteger.h"

#include <optional>
#include <algorithm>

#include <winrt/Citrine.h>

#include <winrt/Windows.Web.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Storage.Streams.h>

namespace winrt {

	using namespace Windows::Foundation;
	using namespace Windows::Web;
	using namespace Windows::Web::Http;
	using namespace Windows::Web::Http::Headers;
	using namespace Windows::Web::Http::Filters;
	using namespace Windows::Storage::Streams;
}

using namespace Citrine;

namespace {

	auto BuildRequestMessage(HttpMethod method, Url const& url, HttpContentParameter content, HttpHeaderParameters const& headers) -> winrt::HttpRequestMessage {

		auto requestMethod = winrt::HttpMethod{ nullptr };
		switch (method) {
		case HttpMethod::Delete:	requestMethod = winrt::HttpMethod::Delete();	break;
		case HttpMethod::Get:		requestMethod = winrt::HttpMethod::Get();		break;
		case HttpMethod::Head:		requestMethod = winrt::HttpMethod::Head();		break;
		case HttpMethod::Options:	requestMethod = winrt::HttpMethod::Options();	break;
		case HttpMethod::Patch:		requestMethod = winrt::HttpMethod::Patch();		break;
		case HttpMethod::Post:		requestMethod = winrt::HttpMethod::Post();		break;
		case HttpMethod::Put:		requestMethod = winrt::HttpMethod::Put();		break;
		default: throw winrt::hresult_invalid_argument{ L"Invalid http method" };
		}

		auto request = winrt::HttpRequestMessage{ requestMethod, winrt::Uri{ winrt::to_hstring(url.RawUrl()) } };
		auto requestHeaders = request.Headers();

		if (auto initializerList = std::get_if<std::initializer_list<HttpHeaderView>>(&headers)) {

			for (auto const& [name, value] : *initializerList)
				requestHeaders.TryAppendWithoutValidation(winrt::to_hstring(name), winrt::to_hstring(value));
		}
		else if (auto collection = std::get_if<std::reference_wrapper<HttpHeaderCollection const>>(&headers)) {

			for (auto const& [name, value] : collection->get())
				requestHeaders.TryAppendWithoutValidation(winrt::to_hstring(name), winrt::to_hstring(value));
		}

		if (!content)
			return request;

		auto requestContent = winrt::IHttpContent{ nullptr };

		if (auto str = std::get_if<std::string>(&content->Payload)) {

			requestContent = winrt::HttpBufferContent{ MakeWinRTBuffer(std::move(*str)) };
		}
		else if (auto vec = std::get_if<std::vector<std::uint8_t>>(&content->Payload)) {

			requestContent = winrt::HttpBufferContent{ MakeWinRTBuffer(std::move(*vec)) };
		}

		if (requestContent) {

			auto contentHeaders = requestContent.Headers();
			for (auto const& [name, value] : content->Headers)
				contentHeaders.Insert(winrt::to_hstring(name), winrt::to_hstring(value));

			request.Content(requestContent);
		}

		return request;
	}

	auto RebuildRequestMessage(winrt::HttpRequestMessage const& oldRequest) -> winrt::HttpRequestMessage {

		auto request = winrt::HttpRequestMessage{};
		request.Method(oldRequest.Method());
		request.RequestUri(oldRequest.RequestUri());
		request.Content(oldRequest.Content());

		for (auto const& [name, value] : oldRequest.Headers())
			request.Headers().TryAppendWithoutValidation(name, value);

		return request;
	}

	auto const RangeHeaderName = winrt::hstring{ L"Range" };

	auto SetRangeHeader(winrt::HttpRequestHeaderCollection const& headers, std::uint64_t from, std::uint64_t to) -> void {

		constexpr auto bufferSize = 48uz; // unit(5) + equals(1) + u64(20) + hyphen(1) + u64(20) + null(1);
		auto buffer = TrivialArray<wchar_t, bufferSize>{};
		auto out = buffer.data();

		out = std::ranges::copy_n(L"bytes=", 6, out).out;
		out = std::ranges::copy(FormatInteger<wchar_t>(from), out).out;
		*out++ = L'-';
		out = std::ranges::copy(FormatInteger<wchar_t>(to), out).out;
		*out = L'\0';

		headers.Insert(RangeHeaderName, std::wstring_view{ buffer.data(), out });
	}

	auto ToHeaderCollection(auto const& headers) -> HttpHeaderCollection {

		auto headerCollection = HttpHeaderCollection{};
		headerCollection.Reserve(headers.Size());

		for (auto const& [name, value] : headers)
			headerCollection.Insert(winrt::to_string(name), winrt::to_string(value));

		return headerCollection;
	}

	auto ToError(winrt::hresult_error const& e) -> HttpError {

		auto errorCode = static_cast<HttpErrorCode>(winrt::WebError::GetStatus(e.code()));
		if (errorCode == HttpErrorCode::Unknown)
			errorCode = static_cast<HttpErrorCode>(e.code().value);

		return { errorCode, winrt::to_string(e.message()) };
	}

	auto client = [] static {

		auto filter = winrt::HttpBaseProtocolFilter();
		auto cacheControl = filter.CacheControl();

		cacheControl.ReadBehavior(winrt::HttpCacheReadBehavior::MostRecent);

		return winrt::HttpClient{ filter };
	}();

	auto streamingClient = [] static {

		auto filter = winrt::HttpBaseProtocolFilter();
		auto cacheControl = filter.CacheControl();

		filter.MaxConnectionsPerServer(64);
		cacheControl.ReadBehavior(winrt::HttpCacheReadBehavior::NoCache);
		cacheControl.WriteBehavior(winrt::HttpCacheWriteBehavior::NoCache);

		return winrt::HttpClient{ filter };
	}();

	struct RandomAccessContentStream : winrt::implements<
		RandomAccessContentStream,
		winrt::IRandomAccessStream,
		winrt::IInputStream,
		winrt::IOutputStream,
		winrt::Citrine::IRangeStreamProvider,
		winrt::IClosable>
	{
		RandomAccessContentStream(winrt::HttpRequestMessage const& requestMessage, std::uint64_t size)

			: requestMessage(requestMessage)
			, size(size)
			, position(0)
		{}

		auto Size() const noexcept -> std::uint64_t {

			return size;
		}

		auto Size(std::uint64_t) -> void {

			throw winrt::hresult_not_implemented{};
		}

		auto Position() const noexcept -> std::uint64_t {

			return position;
		}

		auto Seek(std::uint64_t newPos) -> void {

			if (!IsOpen())
				throw winrt::hresult_error{ RO_E_CLOSED };

			position = newPos;
		}

		auto CanRead() const noexcept -> bool {

			return IsOpen();
		}

		auto ReadAsync(winrt::IBuffer buffer, std::uint32_t count, winrt::InputStreamOptions) -> winrt::IAsyncOperationWithProgress<winrt::IBuffer, std::uint32_t> {

			if (!IsOpen())
				throw winrt::hresult_error{ RO_E_CLOSED };

			if (buffer.Capacity() < count)
				throw winrt::hresult_invalid_argument{};

			if (count == 0 || position >= size) {

				buffer.Length(0);
				co_return buffer;
			}

			auto strongSelf = get_strong();
			auto cancellationToken = co_await winrt::get_cancellation_token();
			cancellationToken.enable_propagation();

			auto const actualCount = std::min(std::uint64_t{ count }, size - position);
			auto const firstBytePos = position;
			auto const lastBytePos = position + actualCount - 1;

			requestMessage = RebuildRequestMessage(requestMessage);
			SetRangeHeader(requestMessage.Headers(), firstBytePos, lastBytePos);

			auto attempts = 3;
			while (attempts-- > 0) {

				try {

					auto completionOption = actualCount > 0x10000 ? winrt::HttpCompletionOption::ResponseHeadersRead : winrt::HttpCompletionOption::ResponseContentRead;
					auto responseMessage = co_await streamingClient.SendRequestAsync(requestMessage, completionOption);
					responseMessage.EnsureSuccessStatusCode();

					auto responseContent = responseMessage.Content();
					auto responseContentStream = co_await responseContent.ReadAsInputStreamAsync();
					auto responseContentBuffer = co_await responseContentStream.ReadAsync(buffer, count, {});

					position += responseContentBuffer.Length();
					co_return responseContentBuffer;
				}
				catch (winrt::hresult_error const& e) {

					auto url = winrt::to_string(requestMessage.RequestUri().RawUri());
					if (attempts > 0 && e.code() == 0x80072eff) { // WININET_E_CONNECTION_RESET

						Logger::Warn("Invoking GET {} with range({} - {}) failed: The connection to the server has been reset, preparing retry", url, firstBytePos, lastBytePos);
						requestMessage = RebuildRequestMessage(requestMessage);
						continue;
					}

					Logger::Error("Invoking GET {} with range({} - {}) failed: {}", url, firstBytePos, lastBytePos, ToError(e));
					throw e;
				}
			}
		}

		auto CanWrite() const noexcept -> bool {

			return false;
		}

		auto WriteAsync(winrt::IBuffer const&) -> winrt::IAsyncOperationWithProgress<std::uint32_t, std::uint32_t>  {

			throw winrt::hresult_not_implemented{};
		}

		auto FlushAsync() -> winrt::Windows::Foundation::IAsyncOperation<bool> {

			throw winrt::hresult_not_implemented{};
		}

		auto GetInputStreamAt(std::uint64_t startPos) -> winrt::Windows::Storage::Streams::IInputStream {

			if (!IsOpen())
				throw winrt::hresult_error{ RO_E_CLOSED };

			auto clonedStream = winrt::make_self<RandomAccessContentStream>(requestMessage, size);
			clonedStream->Seek(startPos);
			return *clonedStream;
		}

		auto GetOutputStreamAt(std::uint64_t) -> winrt::Windows::Storage::Streams::IOutputStream {

			throw winrt::hresult_not_implemented{};
		}

		auto CloneStream() -> winrt::Windows::Storage::Streams::IRandomAccessStream {

			if (!IsOpen())
				throw winrt::hresult_error{ RO_E_CLOSED };

			return winrt::make<RandomAccessContentStream>(requestMessage, size);
		}

		auto GetRangeStream(std::uint64_t offset, std::uint64_t size) -> winrt::IInputStream;

		auto Close() noexcept -> void {

			requestMessage = nullptr;
		}

	private:

		auto IsOpen() const noexcept -> bool {

			return static_cast<bool>(requestMessage);
		}

		struct RangeStreamImpl;

		winrt::HttpRequestMessage requestMessage{ nullptr };
		std::uint64_t size{};
		std::uint64_t position{};
	};

	struct RandomAccessContentStream::RangeStreamImpl : winrt::implements<
		RangeStreamImpl,
		winrt::IInputStream,
		winrt::IClosable>
	{
		RangeStreamImpl(winrt::HttpRequestMessage const& requestMessage, std::uint64_t offset, std::uint64_t size)

			: requestMessage(requestMessage)
			, offset(offset)
			, position(0)
			, size(size)
		{}

		auto ReadAsync(winrt::IBuffer buffer, std::uint32_t count, winrt::InputStreamOptions options) -> winrt::IAsyncOperationWithProgress<winrt::IBuffer, std::uint32_t> {

			if (!IsOpen())
				throw winrt::hresult_error{ RO_E_CLOSED };

			if (buffer.Capacity() < count)
				throw winrt::hresult_invalid_argument{};

			if (count == 0 || position >= size) {

				buffer.Length(0);
				co_return buffer;
			}

			auto strongSelf = get_strong();
			auto cancellationToken = co_await winrt::get_cancellation_token();
			cancellationToken.enable_propagation();

			auto attempts = 3;
			while (attempts-- > 0) {

				try {

					if (!stream) {

						requestMessage = RebuildRequestMessage(requestMessage);
						SetRangeHeader(requestMessage.Headers(), offset + position, offset + size - 1);

						auto completionOption = size - position > 0x10000 ? winrt::HttpCompletionOption::ResponseHeadersRead : winrt::HttpCompletionOption::ResponseContentRead;
						auto responseMessage = co_await streamingClient.SendRequestAsync(requestMessage, completionOption);
						responseMessage.EnsureSuccessStatusCode();

						auto responseContent = responseMessage.Content();
						stream = co_await responseContent.ReadAsInputStreamAsync();
					}

					auto responseContentBuffer = co_await stream.ReadAsync(buffer, count, options);
					position += responseContentBuffer.Length();
					co_return responseContentBuffer;
				}
				catch (winrt::hresult_error const& e) {

					auto url = winrt::to_string(requestMessage.RequestUri().RawUri());
					if (attempts > 0 && e.code() == 0x80072eff) { // WININET_E_CONNECTION_RESET

						Logger::Warn("Invoking GET {} failed: The connection to the server has been reset, preparing retry", url);
						stream = nullptr;
						continue;
					}

					Logger::Error("Invoking GET {} failed: {}", url, ToError(e));
					throw e;
				}
			}
		}

		auto Close() noexcept -> void {

			requestMessage = nullptr;
			stream = nullptr;
		}

	private:

		auto IsOpen() const noexcept -> bool {

			return static_cast<bool>(requestMessage);
		}

		winrt::HttpRequestMessage requestMessage{ nullptr };
		winrt::IInputStream stream{ nullptr };
		std::uint64_t offset{};
		std::uint64_t position{};
		std::uint64_t size{};
	};

	auto RandomAccessContentStream::GetRangeStream(std::uint64_t offset, std::uint64_t size) -> winrt::IInputStream {

		return winrt::make<RangeStreamImpl>(requestMessage, offset, size);
	}
}

namespace Citrine {

	HttpContentParameter::HttpContentParameter(HttpContent&& content) noexcept

		: ptr(&content)
	{}

	HttpContentParameter::operator bool() const noexcept {

		return static_cast<bool>(ptr);
	}

	auto HttpContentParameter::operator*() noexcept -> HttpContent& {

		return *ptr;
	}

	auto HttpContentParameter::operator->() noexcept -> HttpContent* {

		return ptr;
	}

	HttpHeaderParameters::HttpHeaderParameters(std::initializer_list<HttpHeaderView> headers)

		: variant(std::in_place_type<std::initializer_list<HttpHeaderView>>, headers)
	{}

	HttpHeaderParameters::HttpHeaderParameters(HttpHeaderCollection const& headers)

		: variant(std::in_place_type<std::reference_wrapper<HttpHeaderCollection const>>, headers)
	{}

	auto HttpService::SendRequestAsync(HttpMethod method, Url url, HttpContentParameter content, HttpHeaderParameters headers) -> AsyncHttpResult<HttpResponse> try {

		auto requestMessage = BuildRequestMessage(method, url, std::move(content), headers);
		co_await winrt::resume_background();

		auto attempts = 3;
		while (attempts-- > 0) {

			Logger::Info("Invoking {} {}", method, url);
			try {

				auto responseMessage = co_await client.SendRequestAsync(requestMessage);
				auto responseContent = responseMessage.Content();
				auto responseContentHeaders = responseContent.Headers();

				auto response = HttpResponse{

					static_cast<HttpStatusCode>(responseMessage.StatusCode()),
					{ co_await responseContent.ReadAsBufferAsync(), ToHeaderCollection(responseContentHeaders) },
					ToHeaderCollection(responseMessage.Headers())
				};

				Logger::Info("Invoking {} {} completed, status code: {}", method, url, response.StatusCode);
				co_return response;
			}
			catch (winrt::hresult_error const& e) {

				if (attempts > 0 && e.code() == 0x80072eff) { // WININET_E_CONNECTION_RESET

					Logger::Warn("Invoking {} {} failed: The connection to the server has been reset, preparing retry", method, url);
					requestMessage = RebuildRequestMessage(requestMessage);
					continue;
				}
				throw e;
			}
		}
	}
	catch (winrt::hresult_error const& e) {

		auto error = ToError(e);
		Logger::Error("Invoking {} {} failed: {}", method, url, error);
		co_return error;
	}

	auto HttpService::GetStreamAsync(Url url, HttpHeaderParameters headers) -> AsyncHttpResult<HttpStreamResponse> try {

		auto requestMessage = BuildRequestMessage(HttpMethod::Get, url, {}, headers);
		co_await winrt::resume_background();

		auto attempts = 3;
		while (attempts-- > 0) {

			Logger::Info("Invoking GET {}", url);
			try {

				auto responseMessage = co_await streamingClient.SendRequestAsync(requestMessage, winrt::HttpCompletionOption::ResponseHeadersRead);
				auto responseContent = responseMessage.Content();
				auto responseContentHeaders = responseContent.Headers();

				auto response = HttpStreamResponse{
					
					static_cast<HttpStatusCode>(responseMessage.StatusCode()),
					{ co_await responseContent.ReadAsInputStreamAsync(), responseContentHeaders.ContentLength(), ToHeaderCollection(responseContentHeaders) },
					ToHeaderCollection(responseMessage.Headers())
				};

				Logger::Info("Invoking GET {} completed, status code: {}", url, response.StatusCode);
				co_return response;
			}
			catch (winrt::hresult_error const& e) {

				if (attempts > 0 && e.code() == 0x80072eff) { // WININET_E_CONNECTION_RESET

					Logger::Warn("Invoking GET {} failed: The connection to the server has been reset, preparing retry", url);
					requestMessage = RebuildRequestMessage(requestMessage);
					continue;
				}
				throw e;
			}
		}
	}
	catch (winrt::hresult_error const& e) {

		auto error = ToError(e);
		Logger::Error("Invoking GET {} failed: {}", url, error);
		co_return error;
	}

	auto HttpService::GetRandomAccessStreamAsync(Url url, HttpHeaderParameters headers) -> AsyncHttpResult<HttpRandomAccessStreamResponse> try {

		auto requestMessage = BuildRequestMessage(HttpMethod::Get, url, {}, headers);
		SetRangeHeader(requestMessage.Headers(), 0, 0);
		co_await winrt::resume_background();

		auto attempts = 3;
		while (attempts-- > 0) {

			Logger::Info("Invoking GET {}", url);
			try {

				auto responseMessage = co_await streamingClient.SendRequestAsync(requestMessage, winrt::HttpCompletionOption::ResponseHeadersRead);
				auto responseContent = responseMessage.Content();
				auto responseContentHeaders = responseContent.Headers();

				constexpr auto insufficientRangeSupport = static_cast<winrt::hresult>(0x80200013); // BG_E_INSUFFICIENT_RANGE_SUPPORT

				auto responseContentRange = responseContentHeaders.ContentRange();
				if (!responseContentRange)
					throw winrt::hresult_error{ insufficientRangeSupport, L"The server does not support the Content-Range header" };

				if (responseContentRange.Unit() != L"bytes")
					throw winrt::hresult_error{ insufficientRangeSupport, L"The server does support the Content-Range header but uses an unsupported unit" };

				auto responseContentSize = responseContentRange.Length();
				if (!responseContentSize)
					throw winrt::hresult_error{ insufficientRangeSupport, L"The server does support the Content-Range header but did not respond with a content length" };

				auto response = HttpRandomAccessStreamResponse{

					static_cast<HttpStatusCode>(responseMessage.StatusCode()),
					{ winrt::make<RandomAccessContentStream>(requestMessage, responseContentSize.GetUInt64()), responseContentSize.GetUInt64(), ToHeaderCollection(responseContentHeaders)},
					ToHeaderCollection(responseMessage.Headers())
				};

				Logger::Info("Invoking GET {} completed, status code: {}", url, response.StatusCode);
				co_return response;
			}
			catch (winrt::hresult_error const& e) {

				if (attempts > 0 && e.code() == 0x80072eff) { // WININET_E_CONNECTION_RESET

					Logger::Warn("Invoking GET {} failed: The connection to the server has been reset, preparing retry", url);
					requestMessage = RebuildRequestMessage(requestMessage);
					continue;
				}
				throw e;
			}
		}
	}
	catch (winrt::hresult_error const& e) {

		auto error = ToError(e);
		Logger::Error("Invoking GET {} failed: {}", url, error);
		co_return error;
	}
}