#pragma once

#include "Core/Codec/UrlCodec.h"
#include "Core/Util/Param.h"

#include <string>
#include <utility>
#include <vector>

namespace Citrine {

	class UrlQuery {
	public:

		using ValueType = std::pair<std::string, std::string>;
		using SizeType = std::size_t;
		using Iterator = std::vector<ValueType>::const_iterator;
		using ConstIterator = std::vector<ValueType>::const_iterator;

		constexpr UrlQuery() noexcept = default;
		UrlQuery(std::initializer_list<std::pair<std::string_view, std::string_view>> init);

		UrlQuery(UrlQuery const&) = default;
		auto operator=(UrlQuery const&) -> UrlQuery& = default;

		UrlQuery(UrlQuery&&) noexcept = default;
		auto operator=(UrlQuery&&) noexcept -> UrlQuery& = default;

		static auto Parse(std::string_view str, UrlSpaceEncoding spaceEncoding = {}) -> std::optional<UrlQuery>;
		static auto Parse(std::string_view str, UrlQuery& query) -> bool;
		static auto Parse(std::string_view str, UrlSpaceEncoding spaceEncoding, UrlQuery& query) -> bool;
		
		auto Serialize(UrlSpaceEncoding spaceEncoding = {}) const -> std::string;
		auto Serialize(std::string& output) const -> void;
		auto Serialize(AppendTo<std::string> output) const -> void;
		auto Serialize(UrlSpaceEncoding spaceEncoding, std::string& output) const -> void;
		auto Serialize(UrlSpaceEncoding spaceEncoding, AppendTo<std::string> output) const -> void;

		auto begin() const noexcept -> ConstIterator;
		auto end() const noexcept -> ConstIterator;

		auto IsEmpty() const noexcept -> bool;
		auto Size() const noexcept -> SizeType;
		auto Reserve(SizeType capacity) -> void;
		auto Capacity() const noexcept -> SizeType;

		auto Add(std::pair<std::string, std::string>&& pair) -> ValueType const&;
		auto Add(std::pair<std::string_view, std::string_view> pair) -> ValueType const&;
		auto Add(StringParameter key, StringParameter value) -> ValueType const&;

		auto Find(std::string_view key) const noexcept -> ConstIterator;
		auto Contains(std::string_view key) const noexcept -> bool;

		auto Remove(ConstIterator it) noexcept -> ConstIterator;
		auto Remove(std::string_view key) noexcept -> SizeType;
		auto Clear() noexcept -> void;

		auto swap(UrlQuery& other) noexcept -> void;

	private:

		friend struct UrlQueryParser;
		friend struct UrlQuerySerializer;

		std::vector<ValueType> values;
	};

	struct UrlQueryParser {

		static auto Parse(std::string_view str, UrlQuery& query) -> bool;
		static auto Parse(std::string_view str, UrlSpaceEncoding spaceEncoding, UrlQuery& query) -> bool;
	};

	struct UrlQuerySerializer {

		static auto Serialize(UrlQuery const& query, std::string& output) -> void;
		static auto Serialize(UrlQuery const& query, AppendTo<std::string> output) -> void;
		static auto Serialize(UrlQuery const& query, UrlSpaceEncoding spaceEncoding, std::string& output) -> void;
		static auto Serialize(UrlQuery const& query, UrlSpaceEncoding spaceEncoding, AppendTo<std::string> output) -> void;
	};
}
