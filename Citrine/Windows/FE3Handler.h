#pragma once

#include "Windows/AppModel.h"

#include "Core/Coroutine/Task.h"
#include "Core/Net/Url.h"
#include "Core/Util/DateTime.h"
#include "Core/Util/Concepts.h"
#include "Core/Util/FormatInteger.h"
#include "Core/Util/Guid.h"

#include <concepts>
#include <string>
#include <expected>
#include <vector>
#include <format>

namespace Citrine::Windows {

	enum struct FE3Error {

		None,
		NetworkError,
		ApiError,
		ResponseError,
		ContentNotFound
	};

	template<typename T = void>
	struct FE3Result : std::expected<T, FE3Error> {

		using FE3Result::expected::expected;

		FE3Result(FE3Error error) noexcept

			: FE3Result::expected(std::unexpect, error)
		{}
	};

	template<typename T = void>
	using AsyncFE3Result = Task<FE3Result<T>>;

	struct FE3Cookie {

		auto IsValid() const noexcept -> bool;

		DateTime Expiration;
		std::string EncryptedData;
	};

	struct FE3UpdateIdentity {

		Guid UpdateId;
		int RevisionNumber{};
	};

	struct FE3PackageMetadata {

		PackageIdentity PackageId;
		bool IsFramework{};
		bool IsBundle{};
	};

	struct FE3UpdateInfo {

		FE3UpdateIdentity UpdateIdentity;
		FE3PackageMetadata PackageMetadata;
	};

	class FE3Handler {
	public:

		static auto GetCookieAsync() -> AsyncFE3Result<FE3Cookie>;
		static auto GetUpdatesAsync(Guid const& categoryId, FE3Cookie const& cookie) -> AsyncFE3Result<std::vector<FE3UpdateInfo>>;
		static auto GetFileUrlAsync(FE3UpdateIdentity const& updateIdentity) -> AsyncFE3Result<Url>;
		static auto GetFileUrlAsync(Guid const& updateId, int revisionNumber) -> AsyncFE3Result<Url>;
	};
}

namespace std {

	template<::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<::Citrine::Windows::FE3Error, CharT> {

		constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

			return ctx.begin();
		}

		auto format(::Citrine::Windows::FE3Error error, auto& ctx) const -> auto {

			using enum ::Citrine::Windows::FE3Error;

			auto str = std::string_view{};
			switch (error) {
			case None:				str = "None";				break;
			case NetworkError:		str = "NetworkError";		break;
			case ApiError:			str = "ApiError";			break;
			case ResponseError:		str = "ResponseError";		break;
			case ContentNotFound:	str = "ContentNotFound";	break;
			}

			if (!str.empty())
				return std::ranges::copy(str, ctx.out()).out;
			else
				return std::ranges::copy(::Citrine::FormatInteger<CharT>(std::to_underlying(error)), ctx.out()).out;
		}
	};
}