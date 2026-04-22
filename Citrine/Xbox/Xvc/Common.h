#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Util/Concepts.h"
#include "Core/Util/FormatInteger.h"

#include <expected>
#include <format>

namespace Citrine::Xbox {

	enum struct XvcError {

		None,
		FileNotOpen,
		StreamNotOpen,
		ReadingFailed,
		WritingFailed,
		ParsingFailed,
		UnsupportedFormat,
		InvalidPackageManifest,
		DataIntegrityViolation
	};

	template<typename T = void>
	struct XvcOperationResult : std::expected<T, XvcError> {

		using XvcOperationResult::expected::expected;

		template<typename E> requires std::same_as<std::remove_cvref_t<E>, XvcError>
		XvcOperationResult(E&& error)

			: XvcOperationResult::expected(std::unexpect, std::forward<E>(error))
		{}
	};

	template<typename T = void>
	using AsyncXvcOperationResult = Task<XvcOperationResult<T>>;
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Xbox::XvcError, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Xbox::XvcError error, auto& ctx) const -> auto {

			using enum ::Citrine::Xbox::XvcError;

			auto str = std::string_view{};
			switch (error) {
			case None:						str = "None";					break;
			case FileNotOpen:				str = "FileNotOpen";			break;
			case StreamNotOpen:				str = "StreamNotOpen";			break;
			case ReadingFailed:				str = "ReadingFailed";			break;
			case WritingFailed:				str = "WritingFailed";			break;
			case ParsingFailed:				str = "ParsingFailed";			break;
			case UnsupportedFormat:			str = "UnsupportedFormat";		break;
			case InvalidPackageManifest:	str = "InvalidPackageManifest";	break;
			case DataIntegrityViolation:	str = "DataIntegrityViolation";	break;
			}

			if (!str.empty())
				return std::ranges::copy(str, ctx.out()).out;
			else
				return std::ranges::copy(::Citrine::FormatInteger<CharT>(std::to_underlying(error)), ctx.out()).out;
		}
	};
}