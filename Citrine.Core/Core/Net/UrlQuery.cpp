#include "pch.h"
#include "UrlQuery.h"

#include "Core/Util/Ascii.h"

namespace Citrine {

	UrlQuery::UrlQuery(std::initializer_list<std::pair<std::string_view, std::string_view>> init) {

		values.reserve(init.size());
		for (auto const& [key, value] : init) {

			values.emplace_back(key, value);
		}
	}

	auto UrlQuery::Parse(std::string_view str, UrlSpaceEncoding spaceEncoding) -> std::optional<UrlQuery> {

		auto query = std::optional<UrlQuery>{ std::in_place };
		if (!UrlQueryParser::Parse(str, spaceEncoding, *query)) query.reset();
		return query;
	}

	auto UrlQuery::Parse(std::string_view str, UrlQuery& query) -> bool {

		return UrlQueryParser::Parse(str, query);
	}

	auto UrlQuery::Parse(std::string_view str, UrlSpaceEncoding spaceEncoding, UrlQuery& query) -> bool {

		return UrlQueryParser::Parse(str, spaceEncoding, query);
	}

	auto UrlQuery::Serialize(UrlSpaceEncoding spaceEncoding) const -> std::string {

		auto str = std::string{};
		UrlQuerySerializer::Serialize(*this, spaceEncoding, str);
		return str;
	}

	auto UrlQuery::Serialize(std::string& output) const -> void {

		UrlQuerySerializer::Serialize(*this, output);
	}

	auto UrlQuery::Serialize(AppendTo<std::string> output) const -> void {

		UrlQuerySerializer::Serialize(*this, AppendTo(output));
	}

	auto UrlQuery::Serialize(UrlSpaceEncoding spaceEncoding, std::string& output) const -> void {

		UrlQuerySerializer::Serialize(*this, spaceEncoding, output);
	}

	auto UrlQuery::Serialize(UrlSpaceEncoding spaceEncoding, AppendTo<std::string> output) const -> void {

		UrlQuerySerializer::Serialize(*this, spaceEncoding, AppendTo(output));
	}

	auto UrlQuery::begin() const noexcept -> ConstIterator {

		return values.begin();
	}

	auto UrlQuery::end() const noexcept -> ConstIterator {

		return values.end();
	}

	auto UrlQuery::IsEmpty() const noexcept -> bool {

		return values.empty();
	}

	auto UrlQuery::Size() const noexcept -> SizeType {

		return values.size();
	}

	auto UrlQuery::Reserve(SizeType capacity) -> void {

		values.reserve(capacity);
	}

	auto UrlQuery::Capacity() const noexcept -> SizeType {

		return values.capacity();
	}

	auto UrlQuery::Add(std::pair<std::string, std::string>&& pair) -> ValueType const& {

		return values.emplace_back(std::move(pair.first), std::move(pair.second));
	}

	auto UrlQuery::Add(std::pair<std::string_view, std::string_view> pair) -> ValueType const& {

		return values.emplace_back(pair.first, pair.second);
	}

	auto UrlQuery::Add(StringParameter key, StringParameter value) -> ValueType const& {

		return values.emplace_back(std::move(key), std::move(value));
	}

	auto UrlQuery::Find(std::string_view key) const noexcept -> ConstIterator {

		for (auto it = values.begin(); it != values.end(); ++it) {

			if (it->first == key)
				return it;
		}
		return values.end();
	}

	auto UrlQuery::Contains(std::string_view key) const noexcept -> bool {

		return Find(key) != values.end();
	}

	auto UrlQuery::Remove(ConstIterator it) noexcept -> ConstIterator {

		return values.erase(it);
	}

	auto UrlQuery::Remove(std::string_view key) noexcept -> SizeType {

		auto removedCount = 0uz;
		for (auto it = values.begin(); it != values.end();) {

			if (it->first == key) {

				it = values.erase(it);
				++removedCount;
			}
			else ++it;
		}
		return removedCount;
	}

	auto UrlQuery::Clear() noexcept -> void {

		values.clear();
	}

	auto UrlQuery::swap(UrlQuery& other) noexcept -> void {

		values.swap(other.values);
	}

	auto UrlQueryParser::Parse(std::string_view str, UrlQuery& query) -> bool {

		return Parse(str, {}, query);
	}

	auto UrlQueryParser::Parse(std::string_view str, UrlSpaceEncoding spaceEncoding, UrlQuery& query) -> bool {

		query.values.clear();
		if (str.empty())
			return true;

		auto it = str.data();
		auto const end = it + str.size();

		constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$'()*+,;:@/?%">{};

		while (true) {

			auto& [key, value] = query.values.emplace_back();

			auto oldPos = it;
			while (it < end && isValidChar(*it)) ++it;

			if (!UrlCodec::Decode({ oldPos, it }, key, spaceEncoding))
				return false;

			if (it == end || *it++ != '=')
				return false;

			oldPos = it;
			while (it < end && isValidChar(*it)) ++it;

			if (!UrlCodec::Decode({ oldPos, it }, value, spaceEncoding))
				return false;

			if (it == end)
				break;

			if (*it++ != '&')
				return false;
		}
		return true;
	}

	auto UrlQuerySerializer::Serialize(UrlQuery const& query, std::string& output) -> void {

		Serialize(query, {}, output);
	}

	auto UrlQuerySerializer::Serialize(UrlQuery const& query, AppendTo<std::string> output) -> void {

		Serialize(query, {}, AppendTo(output));
	}

	auto UrlQuerySerializer::Serialize(UrlQuery const& query, UrlSpaceEncoding spaceEncoding, std::string& output) -> void {

		output.clear();
		Serialize(query, spaceEncoding, AppendTo(output));
	}

	auto UrlQuerySerializer::Serialize(UrlQuery const& query, UrlSpaceEncoding spaceEncoding, AppendTo<std::string> output) -> void {

		auto const oldSize = output->size();

		auto newCapacity = 0uz;
		for (auto const& [key, value] : query.values) {

			newCapacity += key.size() + 2 + value.size();
		}
		newCapacity = newCapacity * 4 / 3;
		newCapacity += oldSize;

		output->reserve(newCapacity);

		for (auto const& [key, value] : query.values) {

			if (output->size() > oldSize)
				output->push_back('&');

			UrlCodec::Encode(key, AppendTo(output), spaceEncoding);
			output->push_back('=');
			UrlCodec::Encode(value, AppendTo(output), spaceEncoding);
		}
	}
}