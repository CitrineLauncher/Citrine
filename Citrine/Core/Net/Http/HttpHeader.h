#pragma once

#include "Core/Util/Param.h"

#include <string>
#include <vector>
#include <concepts>
#include <optional>
#include <utility>

namespace Citrine {

    struct HttpHeader;
    struct HttpHeaderView;

    struct HttpHeader {

        operator HttpHeaderView() const noexcept;

        std::string Name;
        std::string Value;
    };

    struct HttpHeaderView {

        explicit operator HttpHeader() const;

        std::string_view Name;
        std::string_view Value;
    };

    class HttpHeaderCollection {
        
        struct HeaderParameter;

    public:

        using ValueType = HttpHeader;
        using SizeType = std::size_t;
        using Iterator = std::vector<ValueType>::const_iterator;
        using ConstIterator = std::vector<ValueType>::const_iterator;

        struct NodeHandle : std::optional<ValueType> {};

        struct InsertResult {

            ConstIterator Position{};
            bool Inserted{};
        };

        constexpr HttpHeaderCollection() noexcept = default;
        HttpHeaderCollection(std::initializer_list<HttpHeaderView> headers);

        HttpHeaderCollection(HttpHeaderCollection const&) = default;
        auto operator=(HttpHeaderCollection const&) -> HttpHeaderCollection& = default;

        HttpHeaderCollection(HttpHeaderCollection&&) noexcept = default;
        auto operator=(HttpHeaderCollection&&) noexcept -> HttpHeaderCollection& = default;

        auto begin() const noexcept -> ConstIterator;
        auto end() const noexcept -> ConstIterator;

        auto IsEmpty() const noexcept -> bool;
        auto Size() const noexcept -> SizeType;
        auto Reserve(SizeType newCapacity) -> void;
        auto Capacity() const noexcept -> SizeType;

        auto Insert(NodeHandle&& node) -> InsertResult;
        auto Insert(HeaderParameter header) -> InsertResult;
        auto Insert(StringParameter name, StringParameter value) -> InsertResult;

        auto InsertOrAssign(NodeHandle&& node) -> InsertResult;
        auto InsertOrAssign(HeaderParameter header) -> InsertResult;
        auto InsertOrAssign(StringParameter name, StringParameter value) -> InsertResult;

        auto InsertOrAppend(NodeHandle&& node) -> InsertResult;
        auto InsertOrAppend(HeaderParameter header) -> InsertResult;
        auto InsertOrAppend(StringParameter name, StringParameter value) -> InsertResult;

        auto Find(std::string_view name) const noexcept -> ConstIterator;
        auto Contains(std::string_view name) const noexcept -> bool;

        auto Extract(ConstIterator it) noexcept -> NodeHandle;
        auto Extract(std::string_view name) noexcept -> NodeHandle;

        auto Remove(ConstIterator it) noexcept -> ConstIterator;
        auto Remove(std::string_view name) noexcept -> SizeType;
        auto Clear() noexcept -> void;

        auto swap(HttpHeaderCollection& other) noexcept -> void;

    private:

        auto LowerBoundEqual(std::string_view name) const noexcept -> std::pair<ConstIterator, bool>;

        struct HeaderParameter {

            HeaderParameter(HttpHeader&& header)

                : Name(std::move(header.Name))
                , Value(std::move(header.Value))
            {}

            HeaderParameter(std::convertible_to<HttpHeaderView> auto const& header)

                : Name(header.Name)
                , Value(header.Value)
            {}

            HeaderParameter(StringParameter name, StringParameter value)

                : Name(std::move(name))
                , Value(std::move(value))
            {}

            StringParameter Name;
            StringParameter Value;
        };

        std::vector<ValueType> values;
    };
}