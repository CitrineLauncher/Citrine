#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/TrivialArray.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Util/FormatInteger.h"
#include "Core/Util/Hash.h"

#include <optional>
#include <string>
#include <format>
#include <limits>
#include <algorithm>

#include <glaze/json.hpp>

namespace Citrine {

	template<typename Derived, std::size_t N>
	struct VersionNumberBase {

		static consteval auto SegmentCount() noexcept -> std::size_t {

			return N;
		}

		static constexpr auto Parse(std::string_view str) noexcept -> std::optional<Derived>;
		static constexpr auto Parse(std::string_view str, Derived& value) noexcept -> bool;

		static constexpr auto Parse(std::wstring_view str) noexcept -> std::optional<Derived>;
		static constexpr auto Parse(std::wstring_view str, Derived& value) noexcept -> bool;

		template<IsAnyOf<char, wchar_t> CharT = char>
		constexpr auto Format(this Derived const& self) -> std::basic_string<CharT>;

		template<IsAnyOf<char, wchar_t> CharT>
		constexpr auto FormatTo(this Derived const& self, std::basic_string<CharT>& str) -> void;

		template<IsAnyOf<char, wchar_t> CharT>
		explicit constexpr operator std::basic_string<CharT>(this Derived const& self) {

			return self.template Format<CharT>();
		}

		constexpr auto operator<=>(VersionNumberBase const&) const noexcept -> std::strong_ordering = default;
	};

	template<typename T>
	concept IsVersionNumberType = !std::is_reference_v<T> && requires{ []<typename Derived, std::size_t N>(VersionNumberBase<Derived, N>&&) {}(std::declval<std::remove_cv_t<T>>()); };

	template<typename T>
	concept IsVersionNumberSegment = std::unsigned_integral<std::remove_cvref_t<T>>;

	template<std::size_t I>
	struct EmptyVersionNumberSegment {};

	template<typename T1, typename T2, typename T3 = EmptyVersionNumberSegment<0>, typename T4 = EmptyVersionNumberSegment<1>>
	struct VersionNumberSegments {

		static consteval auto MaxFormattedSize() noexcept -> std::size_t {

			auto size = 0uz;
			size += (std::numeric_limits<std::remove_cvref_t<T1>>::digits10 + 1);
			size += (std::numeric_limits<std::remove_cvref_t<T2>>::digits10 + 2);

			if constexpr (IsVersionNumberSegment<T3>)
				size += (std::numeric_limits<std::remove_cvref_t<T3>>::digits10 + 2);

			if constexpr (IsVersionNumberSegment<T4>)
				size += (std::numeric_limits<std::remove_cvref_t<T4>>::digits10 + 2);

			return size;
		}

		T1 First;
		T2 Second;
		[[no_unique_address, msvc::no_unique_address]]
		T3 Third;
		[[no_unique_address, msvc::no_unique_address]]
		T4 Fourth;
	};

	template<typename... Args>
	VersionNumberSegments(Args&...) -> VersionNumberSegments<std::conditional_t<std::is_const_v<Args>, Args, Args&>...>;

	template<IsVersionNumberType T>
	constexpr auto GetVersionNumberSegments(T& version) noexcept -> auto {

		constexpr auto count = T::SegmentCount();
		if constexpr (count == 2) {

			auto& [first, second] = version;
			return VersionNumberSegments{ first, second };
		}
		else if constexpr (count == 3) {

			auto& [first, second, third] = version;
			return VersionNumberSegments{ first, second, third };
		}
		else if constexpr (count == 4) {

			auto& [first, second, third, fourth] = version;
			return VersionNumberSegments{ first, second, third, fourth };
		}
	}

	struct VersionNumberParser {

		template<typename CharT, IsVersionNumberType T>
		static constexpr auto Parse(std::basic_string_view<CharT> str, T& version) -> bool {

			return Parse(str, GetVersionNumberSegments(version));
		}

		template<typename CharT, typename... Segments>
		static constexpr auto Parse(std::basic_string_view<CharT> str, VersionNumberSegments<Segments...> segments) -> bool {

			auto it = str.data();
			auto const end = it + str.size();

			auto parseSegment = [&](auto& value) -> bool {

				auto [ptr, error] = ParseInteger(it, end, value);
				it = ptr;
				return error == ParseIntegerError::None;
			};

			auto matchDot = [&] -> bool {

				auto result = it < end && *it == '.';
				if (result) ++it;
				return result;
			};

			auto& [first, second, third, fourth] = segments;
			auto result = parseSegment(first) && matchDot() && parseSegment(second);
			
			if constexpr (IsVersionNumberSegment<decltype(third)>) {

				result = result && matchDot() && parseSegment(third);
			}

			if constexpr (IsVersionNumberSegment<decltype(fourth)>) {

				result = result && matchDot() && parseSegment(fourth);
			}
			
			return result && it == end;
		}
	};

	struct VersionNumberFormatter {

		template<IsVersionNumberType T>
		static consteval auto MaxFormattedSize(T const& version) noexcept -> std::size_t {

			return decltype(GetVersionNumberSegments(version))::MaxFormattedSize();
		}

		template<typename CharT, IsVersionNumberType T>
		static constexpr auto FormatTo(CharT* out, T const& version) noexcept -> CharT* {

			return FormatTo(out, GetVersionNumberSegments(version));
		}

		template<typename CharT, IsVersionNumberType T>
		static constexpr auto FormatTo(std::basic_string<CharT>& output, T const& version) noexcept -> void {

			auto buffer = TrivialArray<CharT, MaxFormattedSize(version)>{};
			output.assign(buffer.data(), FormatTo(buffer.data(), GetVersionNumberSegments(version)));
		}

		template<typename CharT, typename... Segments>
		static constexpr auto FormatTo(CharT* out, VersionNumberSegments<Segments...> segments) noexcept -> CharT* {

			auto& [first, second, third, fourth] = segments;

			out = std::ranges::copy(FormatInteger(first), out).out;
			*out++ = '.';
			out = std::ranges::copy(FormatInteger(second), out).out;

			if constexpr (IsVersionNumberSegment<decltype(third)>) {

				*out++ = '.';
				out = std::ranges::copy(FormatInteger(third), out).out;
			}

			if constexpr (IsVersionNumberSegment<decltype(fourth)>) {

				*out++ = '.';
				out = std::ranges::copy(FormatInteger(fourth), out).out;
			}

			return out;
		}
	};

	template<typename Derived, std::size_t N>
	inline constexpr auto VersionNumberBase<Derived, N>::Parse(std::string_view str) noexcept -> std::optional<Derived> {

		auto value = std::optional<Derived>{ std::in_place };
		if (!VersionNumberParser::Parse(str, *value)) value.reset();
		return value;
	}

	template<typename Derived, std::size_t N>
	inline constexpr auto VersionNumberBase<Derived, N>::Parse(std::string_view str, Derived& value) noexcept -> bool {

		return VersionNumberParser::Parse(str, value);
	}

	template<typename Derived, std::size_t N>
	inline constexpr auto VersionNumberBase<Derived, N>::Parse(std::wstring_view str) noexcept -> std::optional<Derived> {

		auto value = std::optional<Derived>{ std::in_place };
		if (!VersionNumberParser::Parse(str, *value)) value.reset();
		return value;
	}

	template<typename Derived, std::size_t N>
	inline constexpr auto VersionNumberBase<Derived, N>::Parse(std::wstring_view str, Derived& value) noexcept -> bool {

		return VersionNumberParser::Parse(str, value);
	}

	template<typename Derived, std::size_t N>
	template<IsAnyOf<char, wchar_t> CharT>
	constexpr auto VersionNumberBase<Derived, N>::Format(this Derived const& self) -> std::basic_string<CharT> {

		auto str = std::basic_string<CharT>{};
		VersionNumberFormatter::FormatTo(str, self);
		return str;
	}

	template<typename Derived, std::size_t N>
	template<IsAnyOf<char, wchar_t> CharT>
	constexpr auto VersionNumberBase<Derived, N>::FormatTo(this Derived const& self, std::basic_string<CharT>& str) -> void {

		VersionNumberFormatter::FormatTo(str, self);
	}
}

namespace std {

	template<::Citrine::IsVersionNumberType T, ::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<T, CharT> {

		constexpr auto parse(basic_format_parse_context<CharT>& ctx) -> auto {

			return ctx.begin();
		}

		auto format(T const& version, auto& ctx) const -> auto {

			using namespace ::Citrine;

			auto buffer = TrivialArray<CharT, VersionNumberFormatter::MaxFormattedSize(version)>{};
			auto end = VersionNumberFormatter::FormatTo(buffer.data(), version);
			return std::copy(buffer.data(), end, ctx.out());
		}
	};

	template<::Citrine::IsVersionNumberType T>
	struct hash<T> {

		static constexpr auto operator()(T const& version) noexcept -> std::size_t {

			using namespace ::Citrine;

			auto [first, second, third, fourth] = GetVersionNumberSegments(version);
			auto hash = FNV1a::OffsetBasis;

			hash = FNV1a::AppendValue(hash, first);
			hash = FNV1a::AppendValue(hash, second);

			if constexpr (IsVersionNumberSegment<decltype(third)>) {

				hash = FNV1a::AppendValue(hash, third);
			}

			if constexpr (IsVersionNumberSegment<decltype(fourth)>) {

				hash = FNV1a::AppendValue(hash, fourth);
			}

			return hash;
		}
	};
}

namespace glz {

	template<::Citrine::IsVersionNumberType T>
	struct meta<T> {

		static constexpr auto custom_read = true;
		static constexpr auto custom_write = true;
	};

	template<::Citrine::IsVersionNumberType T>
	struct from<JSON, T>
	{
		template<auto Opts>
		static auto op(T& version, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			if (!VersionNumberParser::Parse(str, version))
				ctx.error = error_code::parse_error;
		}
	};

	template<::Citrine::IsVersionNumberType T>
	struct to<JSON, T>
	{
		template<auto Opts>
		static auto op(T const& version, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			auto buffer = TrivialArray<char, VersionNumberFormatter::MaxFormattedSize(version)>{};
			auto end = VersionNumberFormatter::FormatTo(buffer.data(), version);
			serialize<JSON>::op<Opts>(std::string_view{ buffer.data(), end }, args...);
		}
	};
}