#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Util/Concepts.h"
#include "Core/Util/FormatInteger.h"

#include <expected>
#include <format>

namespace Citrine::Windows {

	enum struct MsixError {

		None,
		FileNotOpen,
		StreamNotOpen,
		ReadingFailed,
		WritingFailed,
		ParsingFailed,
		UnsupportedFormat,
		InvalidPackageManifest,
		DataIntegrityViolation,
		DecompressionFailed,
	};

	template<typename T = void>
	struct MsixOperationResult : std::expected<T, MsixError> {

		using MsixOperationResult::expected::expected;

		template<typename E> requires std::same_as<std::remove_cvref_t<E>, MsixError>
		MsixOperationResult(E&& error)

			: MsixOperationResult::expected(std::unexpect, std::forward<E>(error))
		{}
	};

	template<typename T = void>
	using AsyncMsixOperationResult = Task<MsixOperationResult<T>>;
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Windows::MsixError, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Windows::MsixError error, auto& ctx) const -> auto {

			using enum ::Citrine::Windows::MsixError;

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
			case DecompressionFailed:		str = "DecompressionFailed";	break;
			}

			if (!str.empty())
				return std::ranges::copy(str, ctx.out()).out;
			else
				return std::ranges::copy(::Citrine::FormatInteger<CharT>(std::to_underlying(error)), ctx.out()).out;
		}
	};
}