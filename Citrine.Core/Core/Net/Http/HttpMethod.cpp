#include "pch.h"
#include "HttpMethod.h"

#include "Core/Util/Frozen.h"

namespace Citrine {

	HttpMethod::HttpMethod(std::string_view name) noexcept {

		static constexpr auto knownMethods = MakeFrozenMap<std::string_view, HttpMethod>({

			{ "DELETE", Delete },
			{ "GET", Get },
			{ "HEAD", Head },
			{ "OPTIONS", Options },
			{ "PATCH", Patch },
			{ "POST", Post },
			{ "PUT", Put },
		});

		auto it = knownMethods.find(name);
		if (it != knownMethods.end())
			value = it->second;
	}

	auto HttpMethod::Name() const noexcept -> std::string_view {

		auto name = std::string_view{ "<unknown>" };
		switch (value) {
		case Delete:	name = "DELETE";	break;
		case Get:		name = "GET";		break;
		case Head:		name = "HEAD";		break;
		case Options:	name = "OPTIONS";	break;
		case Patch:		name = "PATCH";		break;
		case Post:		name = "POST";		break;
		case Put:		name = "PUT";		break;
		}
		return name;
	}

	HttpMethod::operator std::string() const {

		return std::string{ Name() };
	}
}