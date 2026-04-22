#pragma once

#include "Core/Net/Http/Common.h"
#include "Core/Net/Http/HttpMethod.h"
#include "Core/Net/Http/HttpContent.h"
#include "Core/Net/Http/HttpHeader.h"
#include "Core/Net/Http/HttpResponse.h"

#include "Core/Net/Url.h"

#include <variant>
#include <functional>

namespace Citrine {

	class HttpContentParameter {
	public:

		HttpContentParameter() noexcept = default;
		HttpContentParameter(HttpContent&& content) noexcept;

		HttpContentParameter(HttpContentParameter const&) = delete;
		auto operator=(HttpContentParameter const&) = delete;

		HttpContentParameter(HttpContentParameter&&) noexcept = default;

		operator bool() const noexcept;
		auto operator*() noexcept -> HttpContent&;
		auto operator->() noexcept -> HttpContent*;

	private:

		HttpContent* ptr{ nullptr };
	};

	class HttpHeaderParameters : public std::variant<
		std::initializer_list<HttpHeaderView>,
		std::reference_wrapper<HttpHeaderCollection const>>
	{
	public:

		HttpHeaderParameters(std::initializer_list<HttpHeaderView> headers);
		HttpHeaderParameters(HttpHeaderCollection const& headers);
	};

	class HttpService {
	public:

		static auto SendRequestAsync(HttpMethod method, Url url, HttpContentParameter content = {}, HttpHeaderParameters headers = {}) -> AsyncHttpResult<HttpResponse>;
		static auto GetStreamAsync(Url url, HttpHeaderParameters headers = {}) -> AsyncHttpResult<HttpStreamResponse>;
		static auto GetRandomAccessStreamAsync(Url url, HttpHeaderParameters headers = {}) -> AsyncHttpResult<HttpRandomAccessStreamResponse>;
	};
}