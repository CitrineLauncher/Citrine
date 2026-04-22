#include "pch.h"
#include "LogFormatter.h"

#include "Core/Unicode/Utf.h"
#include "Core/Util/StringLiteral.h"

namespace Citrine {

	template<IsAnyOf<char, wchar_t> CharT>
	auto LogFormatter<std::filesystem::path, CharT>::format(std::filesystem::path const& value, BasicLogFormatContext<CharT>& ctx) const -> decltype(ctx.out()) {

		constexpr auto isSlash = [](auto ch) static -> bool {

			return ch == '\\' || ch == '/';
		};

		decltype(auto) str = [this, &value] -> decltype(auto) {

			if constexpr (std::same_as<CharT, std::filesystem::path::value_type>)
				return value.native();
			else
				return value.string<CharT>();
		}();

		auto const begin = str.c_str();
		auto const end = begin + str.size();

		auto it = begin;
		auto out = ctx.out();

		while (it < end && *it++ != ':');
		while (isSlash(*it)) ++it; //null terminated

		static constexpr auto usersDirectory = StringLiteralCast<CharT>("Users");

		if (end - it >= 7 && std::basic_string_view{ it, 5 } == usersDirectory && isSlash(it[5])) {

			it += 6;
			while (isSlash(*it)) ++it; //null terminated
		}
		else { return std::ranges::copy(str, out).out; }

		auto const usernameBegin = it;
		while (it < end && !isSlash(*it)) ++it;

		out = std::copy(begin, usernameBegin, out);
		if (it - usernameBegin > 0) {

			out = std::copy_n("<User>", 6, out);
			out = std::copy(it, end, out);
		}
		return out;
	}

	template struct LogFormatter<std::filesystem::path, char>;
	template struct LogFormatter<std::filesystem::path, wchar_t>;
}