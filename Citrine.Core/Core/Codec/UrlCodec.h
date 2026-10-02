#pragma once

#include "Core/Util/Append.h"

#include <string>

namespace Citrine {

	enum struct UrlSpaceEncoding {

		PercentEscaped,
		PlusReplaced
	};

	struct UrlCodec {

		static auto Encode(std::string_view input, std::string& output, UrlSpaceEncoding spaceEncoding = {}) -> void;
		static auto Encode(std::string_view input, AppendTo<std::string> output, UrlSpaceEncoding spaceEncoding = {}) -> void;

		static auto Decode(std::string_view input, std::string& output, UrlSpaceEncoding spaceEncoding = {}) -> bool;
		static auto Decode(std::string_view input, AppendTo<std::string> output, UrlSpaceEncoding spaceEncoding = {}) -> bool;

		static auto Validate(std::string_view input) noexcept -> bool;
	};
}
