#include "pch.h"
#include "HttpHeader.h"

#include "Core/Util/Ascii.h"

#include <algorithm>

namespace {

    auto AppendToHeader(Citrine::HttpHeader& header, std::string_view value) {

        auto oldSize = header.Value.size();
        auto newSize = oldSize + 2 + value.size();

        header.Value.resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

            auto out = data + oldSize;
            out = std::ranges::copy_n(", ", 2, out).out;
            out = std::ranges::copy(value, out).out;
            return size;
        });
    }
}

namespace Citrine {

    HttpHeader::operator HttpHeaderView() const noexcept {

        return { Name, Value };
    }

    HttpHeaderView::operator HttpHeader() const {

        return { std::string{ Name }, std::string{ Value } };
    }

    HttpHeaderCollection::HttpHeaderCollection(std::initializer_list<HttpHeaderView> headers) {

        values.reserve(headers.size());
        for (auto const& [name, value] : headers) {

            InsertOrAppend(name, value);
        }
    }

    auto HttpHeaderCollection::begin() const noexcept -> ConstIterator {

        return values.begin();
    }

    auto HttpHeaderCollection::end() const noexcept -> ConstIterator {

        return values.end();
    }

    auto HttpHeaderCollection::IsEmpty() const noexcept -> bool {

        return values.empty();
    }

    auto HttpHeaderCollection::Size() const noexcept -> SizeType {

        return values.size();
    }

    auto HttpHeaderCollection::Reserve(SizeType newCapacity) -> void {

        values.reserve(newCapacity);
    }

    auto HttpHeaderCollection::Capacity() const noexcept -> SizeType {

        return values.capacity();
    }

    auto HttpHeaderCollection::Insert(NodeHandle&& node) -> InsertResult {

        if (node)
            return Insert(std::move(node->Name), std::move(node->Value));
        return { values.end() };
    }

    auto HttpHeaderCollection::Insert(HeaderParameter header) -> InsertResult {

        return Insert(std::move(header.Name), std::move(header.Value));
    }

    auto HttpHeaderCollection::Insert(StringParameter name, StringParameter value) -> InsertResult {

        auto [it, equal] = LowerBoundEqual(name.View());
        if (equal) {

            return { it };
        }
        return { values.emplace(it, std::move(name), std::move(value)), true };
    }

    auto HttpHeaderCollection::InsertOrAssign(NodeHandle&& node) -> InsertResult {

        if (node)
            return InsertOrAssign(std::move(node->Name), std::move(node->Value));
        return { values.end() };
    }

    auto HttpHeaderCollection::InsertOrAssign(HeaderParameter header) -> InsertResult {

        return InsertOrAssign(std::move(header.Name), std::move(header.Value));
    }

    auto HttpHeaderCollection::InsertOrAssign(StringParameter name, StringParameter value) -> InsertResult {

        auto [it, equal] = LowerBoundEqual(name.View());
        if (equal) {

            const_cast<ValueType&>(*it).Value = std::move(value);
            return { it };
        }
        return { values.emplace(it, std::move(name), std::move(value)), true };
    }

    auto HttpHeaderCollection::InsertOrAppend(NodeHandle&& node) -> InsertResult {

        if (node)
            return InsertOrAppend(std::move(node->Name), std::move(node->Value));
        return { values.end() };
    }

    auto HttpHeaderCollection::InsertOrAppend(HeaderParameter header) -> InsertResult {

        return InsertOrAppend(std::move(header.Name), std::move(header.Value));
    }

    auto HttpHeaderCollection::InsertOrAppend(StringParameter name, StringParameter value) -> InsertResult {

        auto [it, equal] = LowerBoundEqual(name.View());
        if (equal) {

            AppendToHeader(const_cast<ValueType&>(*it), value.View());
            return { it };
        }
        return { values.emplace(it, std::move(name), std::move(value)), true };
    }

    auto HttpHeaderCollection::Find(std::string_view name) const noexcept -> ConstIterator {

        auto [it, equal] = LowerBoundEqual(name);
        return equal ? it : values.end();
    }

    auto HttpHeaderCollection::Contains(std::string_view name) const noexcept -> bool {

        auto [it, equal] = LowerBoundEqual(name);
        return equal;
    }

    auto HttpHeaderCollection::Extract(ConstIterator it) noexcept -> NodeHandle {

        auto node = NodeHandle{ std::move(const_cast<ValueType&>(*it)) };
        values.erase(it);
        return node;
    }

    auto HttpHeaderCollection::Extract(std::string_view name) noexcept -> NodeHandle {

        auto node = NodeHandle{};
        auto [it, equal] = LowerBoundEqual(name);
        if (equal) {

            node.emplace(std::move(const_cast<ValueType&>(*it)));
            values.erase(it);
        }
        return node;
    }

    auto HttpHeaderCollection::Remove(ConstIterator it) noexcept -> ConstIterator {

        return values.erase(it);
    }

    auto HttpHeaderCollection::Remove(std::string_view name) noexcept -> SizeType {

        auto [it, equal] = LowerBoundEqual(name);
        if (equal) {

            values.erase(it);
        }
        return static_cast<SizeType>(equal);
    }

    auto HttpHeaderCollection::Clear() noexcept -> void {

        values.clear();
    }

    auto HttpHeaderCollection::swap(HttpHeaderCollection& other) noexcept -> void {

        values.swap(other.values);
    }

    auto HttpHeaderCollection::LowerBoundEqual(std::string_view name) const noexcept -> std::pair<ConstIterator, bool> {

        constexpr auto toLower = [](char ch) static { return Ascii::ToLower(ch); };
        auto it = values.begin();
        auto count = values.size();

        while (count > 0) {

            auto const half = count / 2;
            auto const mid = it + half;

            if (std::ranges::lexicographical_compare(mid->Name, name, {}, toLower, toLower)) {

                it = mid + 1;
                count -= half + 1;
            }
            else {

                count = half;
            }
        }
        return { it, it != values.end() && std::ranges::equal(it->Name, name, {}, toLower, toLower) };
    }
}