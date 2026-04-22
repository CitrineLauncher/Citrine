#pragma once

#include <string>

#include "Core/Util/Append.h"

namespace Citrine {

	auto ToUtf8(std::wstring_view input) -> std::string;
	auto ToUtf8(std::wstring_view input, std::string& output) -> void;
	auto ToUtf8(std::wstring_view input, AppendTo<std::string> output) -> void;

	auto ToUtf16(std::string_view input) -> std::wstring;
	auto ToUtf16(std::string_view input, std::wstring& output) -> void;
	auto ToUtf16(std::string_view input, AppendTo<std::wstring> output) -> void;
}
