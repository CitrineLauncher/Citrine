#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Util/Guid.h"
#include "Windows/AppModel.h"

#include <optional>
#include <format>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine::Xbox {

	struct PackageVersionIdentifier {

		struct Parser;
		struct Formatter;

		static auto Parse(std::string_view str) noexcept -> std::optional<PackageVersionIdentifier>;
		static auto Parse(std::string_view str, PackageVersionIdentifier& value) noexcept -> bool;

		auto Format() const -> std::string;
		auto FormatTo(std::string& str) const -> void;

		explicit operator std::string() const;

		constexpr auto operator<=>(PackageVersionIdentifier const&) const noexcept -> std::strong_ordering = default;

		Windows::PackageVersion Version;
		Guid Id{};
	};

	struct PackageVersionIdentifier::Parser {

		static auto Parse(std::string_view str, PackageVersionIdentifier& value) noexcept -> bool;
	};

	struct PackageVersionIdentifier::Formatter {

		static consteval auto MaxFormattedSize() -> std::size_t {

			return VersionNumberFormatter::MaxFormattedSize<Windows::PackageVersion>() + 1 + GuidFormatter::FormattedSize();
		}

		static auto FormatTo(char* out, PackageVersionIdentifier const& value) noexcept -> char*;
		static auto FormatTo(std::string& output, PackageVersionIdentifier const& value) -> void;
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Xbox::PackageVersionIdentifier, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Xbox::PackageVersionIdentifier const& identifier, auto& ctx) const -> auto {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, FormatterT::MaxFormattedSize()>{};
			auto end = FormatterT::FormatTo(buffer.data(), identifier);
			return std::copy(buffer.data(), end, ctx.out());
		}

	protected:

		using FormatterT = ::Citrine::Xbox::PackageVersionIdentifier::Formatter;
	};
}

namespace glz {

	template<>
	struct meta<::Citrine::Xbox::PackageVersionIdentifier> {

		using mimic = std::string;
	};

	template<>
	struct from<JSON, ::Citrine::Xbox::PackageVersionIdentifier>
	{
		template<auto Opts>
		static auto op(::Citrine::Xbox::PackageVersionIdentifier& identifier, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;
			using ParserT = Xbox::PackageVersionIdentifier::Parser;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			if (!ParserT::Parse(str, identifier))
				ctx.error = error_code::parse_error;
		}
	};

	template<>
	struct to<JSON, ::Citrine::Xbox::PackageVersionIdentifier>
	{
		template<auto Opts>
		static auto op(::Citrine::Xbox::PackageVersionIdentifier const& identifier, auto&&... args) noexcept -> void {

			using namespace ::Citrine;
			using FormatterT = Xbox::PackageVersionIdentifier::Formatter;

			auto buffer = TrivialArray<char, FormatterT::MaxFormattedSize()>{};
			auto end = FormatterT::FormatTo(buffer.data(), identifier);
			serialize<JSON>::op<Opts>(std::string_view{ buffer.data(), end }, args...);
		}
	};
}