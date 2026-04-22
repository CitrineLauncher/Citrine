#pragma once

#include <type_traits>
#include <concepts>
#include <utility>
#include <iterator>
#include <ranges>
#include <functional>
#include <string_view>
#include <array>
#include <bit>
#include <tuple>

namespace Citrine {
	
	struct FNV1a {

		static constexpr auto OffsetBasis = sizeof(std::size_t) >= 8
			? 0xcbf29ce484222325uz
			: 0x811c9dc5uz;
		static constexpr auto Prime = sizeof(std::size_t) >= 8
			? 0x100000001b3uz
			: 0x1000193uz;

		template<typename T>
			requires std::has_unique_object_representations_v<T>
		static constexpr auto AppendValue(std::size_t hash, T const& value) noexcept -> std::size_t {

			if consteval {

				auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
				return AppendBytes(hash, bytes.data(), bytes.size());
			}
			else {

				return AppendBytes(hash, reinterpret_cast<std::uint8_t const*>(&value), sizeof(T));
			}
		}

		template<std::input_iterator It>
			requires std::has_unique_object_representations_v<std::iter_value_t<It>>
		static constexpr auto AppendRange(std::size_t hash, It first, It last) noexcept -> std::size_t {

			using T = std::iter_value_t<It>;

			if consteval {

				for (auto it = first; it != last; ++it) {

					auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(*it);
					hash = AppendBytes(hash, bytes.data(), bytes.size());
				}
			}
			else {

				if constexpr (std::contiguous_iterator<It>) {

					auto firstB = reinterpret_cast<std::uint8_t const*>(std::to_address(first));
					auto lastB = reinterpret_cast<std::uint8_t const*>(std::to_address(last));
					hash = AppendBytes(hash, firstB, static_cast<std::size_t>(lastB - firstB));
				}
				else {

					for (auto it = first; it != last; ++it) {

						decltype(auto) value = *it;
						hash = AppendBytes(hash, reinterpret_cast<std::uint8_t const*>(&value), sizeof(T));
					}
				}
			}

			return hash;
		}

		template<std::ranges::input_range Range>
			requires std::is_trivially_copyable_v<std::ranges::range_value_t<Range>>
		static constexpr auto AppendRange(std::size_t hash, Range const& range) noexcept -> std::size_t {

			return AppendRange(hash, std::begin(range), std::end(range));
		}

		static constexpr auto AppendBytes(std::size_t hash, std::uint8_t const* data, std::size_t count) noexcept -> std::size_t {

			for (auto i = 0uz; i < count; ++i) {

				hash ^= static_cast<std::size_t>(data[i]);
				hash *= Prime;
			}

			return hash;
		}
	};

	template<typename T>
	constexpr auto HashValue(T const& value)
		noexcept(std::is_scalar_v<T>) -> std::size_t
	{
		if constexpr (std::is_scalar_v<T>)
			return FNV1a::AppendValue(FNV1a::OffsetBasis, value);
		else
			return std::hash<T>{}(value);
	}

	template<typename T>
	constexpr auto HashCombine(std::size_t hash, T const& value)
		noexcept(std::is_scalar_v<T>) -> std::size_t
	{
		if constexpr (sizeof(std::size_t) >= 8)
			hash ^= HashValue(value) + 0x517cc1b727220a95 + (hash << 6) + (hash >> 2);
		else
			hash ^= HashValue(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		return hash;
	}

	template<std::input_iterator It>
	constexpr auto HashRange(It first, It last)
		noexcept(std::is_scalar_v<std::iter_value_t<It>>) -> std::size_t
	{
		using T = std::iter_value_t<It>;

		if constexpr (std::is_scalar_v<T>) {

			return FNV1a::AppendRange(FNV1a::OffsetBasis, first, last);
		}
		else {

			auto hash = 0uz;
			for (auto it = first; it != last; ++it)
				hash = HashCombine(hash, *it);

			return hash;
		}
	}

	template<std::ranges::input_range Range>
	constexpr auto HashRange(Range const& range)
		noexcept(std::is_scalar_v<std::ranges::range_value_t<Range>>) -> std::size_t
	{
		return HashRange(std::begin(range), std::end(range));
	}

	template<typename... Ts>
	constexpr auto HashPair(std::pair<Ts...> const& pair)
		noexcept((std::is_scalar_v<std::remove_cvref_t<Ts>> && ...)) -> std::size_t
	{
		if constexpr ((std::is_scalar_v<std::remove_cvref_t<Ts>> && ...)) {

			auto hash = FNV1a::OffsetBasis;
			hash = FNV1a::AppendValue(hash, pair.first);
			hash = FNV1a::AppendValue(hash, pair.second);
			return hash;
		}
		else {

			auto hash = HashValue(pair.first);
			hash = HashCombine(hash, pair.second);
			return hash;
		}
	}

	template<typename... Ts>
	constexpr auto HashTuple(std::tuple<Ts...> const& tuple)
		noexcept((std::is_scalar_v<std::remove_cvref_t<Ts>> && ...)) -> std::size_t
	{
		if constexpr ((std::is_scalar_v<std::remove_cvref_t<Ts>> && ...)) {

			return [&]<std::size_t... I>(std::index_sequence<I...>) {

				auto hash = FNV1a::OffsetBasis;
				((hash = FNV1a::AppendValue(hash, std::get<I>(tuple))), ...);
				return hash;
			}
			(std::index_sequence_for<Ts...>{});
		}
		else {

			return [&]<std::size_t... I>(std::index_sequence<0, I...>) {

				auto hash = HashValue(std::get<0>(tuple));
				((hash = HashCombine(hash, std::get<I>(tuple))), ...);
				return hash;
			}
			(std::index_sequence_for<Ts...>{});
		}
	}

	template<typename... Hashes>
	struct __declspec(empty_bases) TransparentHash : Hashes...
	{
		using is_transparent = void;
		using Hashes::operator()...;

		using Base = TransparentHash;
	};
}