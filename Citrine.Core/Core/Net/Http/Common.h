#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Coroutine/Task.h"
#include "Core/Unicode/Utf.h"

#include <expected>
#include <format>
#include <utility>

namespace Citrine {

	enum struct HttpErrorCode {

        Unknown = 0,
        CertificateCommonNameIsIncorrect = 1,
        CertificateExpired = 2,
        CertificateContainsErrors = 3,
        CertificateRevoked = 4,
        CertificateIsInvalid = 5,
        ServerUnreachable = 6,
        Timeout = 7,
        ErrorHttpInvalidServerResponse = 8,
        ConnectionAborted = 9,
        ConnectionReset = 10,
        Disconnected = 11,
        HttpToHttpsOnRedirection = 12,
        HttpsToHttpOnRedirection = 13,
        CannotConnect = 14,
        HostNameNotResolved = 15,
        OperationCanceled = 16,
        RedirectFailed = 17,
        UnexpectedStatusCode = 18,
        UnexpectedRedirection = 19,
        UnexpectedClientError = 20,
        UnexpectedServerError = 21,
        InsufficientRangeSupport = 22,
        MissingContentLengthSupport = 23,
        MultipleChoices = 300,
        MovedPermanently = 301,
        Found = 302,
        SeeOther = 303,
        NotModified = 304,
        UseProxy = 305,
        TemporaryRedirect = 307,
        BadRequest = 400,
        Unauthorized = 401,
        PaymentRequired = 402,
        Forbidden = 403,
        NotFound = 404,
        MethodNotAllowed = 405,
        NotAcceptable = 406,
        ProxyAuthenticationRequired = 407,
        RequestTimeout = 408,
        Conflict = 409,
        Gone = 410,
        LengthRequired = 411,
        PreconditionFailed = 412,
        RequestEntityTooLarge = 413,
        RequestUriTooLong = 414,
        UnsupportedMediaType = 415,
        RequestedRangeNotSatisfiable = 416,
        ExpectationFailed = 417,
        InternalServerError = 500,
        NotImplemented = 501,
        BadGateway = 502,
        ServiceUnavailable = 503,
        GatewayTimeout = 504,
        HttpVersionNotSupported = 505
	};

	struct HttpError {

        HttpErrorCode Code{};
		std::string Message;
	};

	template<typename T = void>
	struct HttpResult : std::expected<T, HttpError> {

		using HttpResult::expected::expected;

		template<typename E> requires std::same_as<std::remove_cvref_t<E>, HttpError>
		HttpResult(E&& error)

			: HttpResult::expected(std::unexpect, std::forward<E>(error))
		{}
	};

	template<typename T = void>
	using AsyncHttpResult = Task<HttpResult<T>>;
}

namespace std {

    template<::Citrine::IsAnyOf<char, wchar_t> CharT>
    struct formatter<::Citrine::HttpErrorCode, CharT> : formatter<int, CharT> {

        auto format(::Citrine::HttpErrorCode value, auto& ctx) const -> auto {

            return formatter<int, CharT>::format(std::to_underlying(value), ctx);
        }
    };

    template<::Citrine::IsAnyOf<char, wchar_t> CharT>
    struct formatter<::Citrine::HttpError, CharT> {

        constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

            return ctx.begin();
        }

        auto format(::Citrine::HttpError const& value, auto& ctx) const -> auto {

            using namespace ::Citrine;

            if constexpr (std::same_as<CharT, wchar_t>)
                return format_to(ctx.out(), L"(Code: {}, Message: {})", value.Code, ToUtf16(value.Message));
            else
                return format_to(ctx.out(), "(Code: {}, Message: {})", value.Code, value.Message);
        }
    };
}