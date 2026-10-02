#pragma once

#include "HttpHeader.h"

#include <variant>
#include <string>
#include <vector>
#include <utility>

namespace Citrine {

	using HttpPayload = std::variant<
		std::monostate,
		std::string,
		std::vector<std::uint8_t>
	>;

	struct HttpContent {

		HttpPayload Payload;
		HttpHeaderCollection Headers;
	};

	auto HttpFormUrlEncodedContent(std::initializer_list<std::pair<std::string_view, std::string_view>> params) -> HttpContent;
	auto HttpFormUrlEncodedContent(std::span<std::pair<std::string_view, std::string_view> const> params) -> HttpContent;
	auto HttpFormUrlEncodedContent(std::span<std::pair<std::string, std::string> const> params) -> HttpContent;
}
