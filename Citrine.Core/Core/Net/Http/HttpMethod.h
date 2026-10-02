#pragma once

#include "Core/Util/Concepts.h"

#include <type_traits>
#include <string_view>
#include <format>
#include <algorithm>

namespace Citrine {

    class HttpMethod {
    public:

        enum struct ValueType {};

        static const HttpMethod Unknown;
        static const HttpMethod Delete;
        static const HttpMethod Get;
        static const HttpMethod Head;
        static const HttpMethod Options;
        static const HttpMethod Patch;
        static const HttpMethod Post;
        static const HttpMethod Put;

        constexpr HttpMethod() noexcept = default;

        explicit constexpr HttpMethod(std::underlying_type_t<ValueType> value) noexcept
            
            : value{ value }
        {}

        explicit HttpMethod(std::string_view name) noexcept;

        constexpr HttpMethod(HttpMethod const&) noexcept = default;
        constexpr auto operator=(HttpMethod const&) noexcept -> HttpMethod& = default;

        auto Name() const noexcept -> std::string_view;

        explicit operator std::string() const;

        constexpr operator ValueType() const noexcept {

            return value;
        }

        constexpr auto operator<=>(HttpMethod const&) const noexcept -> std::strong_ordering = default;

    private:

        ValueType value{};
    };

    inline constexpr HttpMethod HttpMethod::Unknown { 0 };
    inline constexpr HttpMethod HttpMethod::Delete  { 1 };
    inline constexpr HttpMethod HttpMethod::Get     { 2 };
    inline constexpr HttpMethod HttpMethod::Head    { 3 };
    inline constexpr HttpMethod HttpMethod::Options { 4 };
    inline constexpr HttpMethod HttpMethod::Patch   { 5 };
    inline constexpr HttpMethod HttpMethod::Post    { 6 };
    inline constexpr HttpMethod HttpMethod::Put     { 7 };
}

namespace std {

    template<::Citrine::IsAnyOf<char, wchar_t> CharT>
    struct formatter<::Citrine::HttpMethod, CharT> {

        constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

            return ctx.begin();
        }

        auto format(::Citrine::HttpMethod method, auto& ctx) const -> auto {

            return std::ranges::copy(method.Name(), ctx.out()).out;
        }
    };
}