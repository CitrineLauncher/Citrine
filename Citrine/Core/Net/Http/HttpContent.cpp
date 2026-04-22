#include "pch.h"
#include "HttpContent.h"

#include "Core/Codec/UrlCodec.h"

namespace {

	using namespace ::Citrine;

	auto EncodeForm(auto params, std::string& output) -> void {

		output.clear();

		auto newCapacity = 0uz;
		for (auto const& [key, value] : params) {

			newCapacity += key.size() + 2 + value.size();
		}
		newCapacity = newCapacity * 4 / 3;

		output.reserve(newCapacity);

		for (auto const& [key, value] : params) {

			if (output.size() > 0)
				output.push_back('&');

			UrlCodec::Encode(key, AppendTo(output), UrlSpaceEncoding::PlusReplaced);
			output.push_back('=');
			UrlCodec::Encode(value, AppendTo(output), UrlSpaceEncoding::PlusReplaced);
		}
	}
}

namespace Citrine {

	auto HttpFormUrlEncodedContent(std::initializer_list<std::pair<std::string_view, std::string_view>> params) -> HttpContent {

		return HttpFormUrlEncodedContent(std::span{ std::data(params), std::size(params) });
	}

	auto HttpFormUrlEncodedContent(std::span<std::pair<std::string_view, std::string_view> const> params) -> HttpContent {

		auto content = HttpContent{};
		EncodeForm(params, content.Payload.emplace<std::string>());
		content.Headers.Insert("Content-Type", "application/x-www-form-urlencoded");
		return content;
	}

	auto HttpFormUrlEncodedContent(std::span<std::pair<std::string, std::string> const> params) -> HttpContent {

		auto content = HttpContent{};
		EncodeForm(params, content.Payload.emplace<std::string>());
		content.Headers.Insert("Content-Type", "application/x-www-form-urlencoded");
		return content;
	}
}