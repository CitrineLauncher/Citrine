#pragma once

#include "Core/Util/SemanticVersion.h"
#include "Core/Util/Concepts.h"
#include "Core/Util/StringLiteral.h"
#include "Core/Unicode/Utf.h"

#include <vector>
#include <format>
#include <algorithm>

#include <glaze/json.hpp>

namespace Citrine {

	enum struct ReleaseChannel : std::uint8_t {

		Unknown,
		Stable,
		Preview
	};

	struct ReleaseInfo {

		SemanticVersion Version;
		ReleaseChannel Channel{};
		std::string Tag;
	};

	using ReleaseInfoCollection = std::vector<ReleaseInfo>;
}

namespace glz {

	template<>
	struct meta<::Citrine::ReleaseChannel> {

		using enum ::Citrine::ReleaseChannel;

		static constexpr auto value = enumerate(
			"Stable", Stable,
			"Preview", Preview
		);
	};

	template<>
	struct meta<::Citrine::ReleaseInfo> {

		using T = ::Citrine::ReleaseInfo;

		static constexpr auto value = object(
			"Version", &T::Version,
			"Channel", &T::Channel,
			"Tag", &T::Tag
		);
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::ReleaseChannel, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::ReleaseChannel channel, auto& ctx) const -> auto {

			using enum ::Citrine::ReleaseChannel;

			auto str = std::string_view{ "<unknown>" };
			switch (channel) {
			case Stable:	str = "Stable";		break;
			case Preview:	str = "Preview";	break;
			}

			return std::ranges::copy(str, ctx.out()).out;
		}
	};

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::ReleaseInfo, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::ReleaseInfo const& info, auto& ctx) const -> auto {

			return format_to(ctx.out(), GetFormatString(), info.Version, info.Channel, GetString(info.Tag));
		}

	private:

		static consteval auto GetFormatString() noexcept -> auto const& {

			static constexpr auto fmt = ::Citrine::StringLiteralCast<CharT>(
				"(Version: {}, Channel: {}, Tag: {})"
			);
			return fmt;
		}

		template<typename Str>
		static auto GetString(Str const& str) -> decltype(auto) {

			if constexpr (std::same_as<CharT, wchar_t>)
				return ToUtf16(str);
			else
				return (str);
		}
	};
}
