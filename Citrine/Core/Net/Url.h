#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Unicode/Utf.h"

#include <string>
#include <format>
#include <filesystem>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine {

	class UrlComponentBase {
	protected:

		struct Component {

			std::uint16_t Position{};
			std::uint16_t Size{};
		};

		struct Components {

			Component Scheme;
			Component User;
			Component Password;
			Component Host;
			std::uint16_t Port{};
			Component Path;
			Component Query;
			Component Fragment;
		};

		static auto Parse(std::string_view url, Components& components) noexcept -> bool;
	};

	template<typename StringType>
	class UrlBase : protected UrlComponentBase {

		static constexpr auto NoThrow = std::is_nothrow_constructible_v<StringType, char const*, std::size_t>;

	public:

		auto Scheme() const noexcept(NoThrow) -> StringType;
		auto User() const noexcept(NoThrow) -> StringType;
		auto Password() const noexcept(NoThrow) -> StringType;
		auto Host() const noexcept(NoThrow) -> StringType;
		auto Port() const noexcept -> std::uint16_t;
		auto Path() const noexcept(NoThrow) -> StringType;
		auto PathAndQuery() const noexcept(NoThrow) -> StringType;
		auto Query() const noexcept(NoThrow) -> StringType;
		auto Fragment() const noexcept(NoThrow) -> StringType;

		auto IsEmpty() const noexcept -> bool;
		auto IsWellFormed() const noexcept -> bool;

	protected:

		constexpr UrlBase() noexcept = default;

		template<std::convertible_to<std::string_view> Arg>
		UrlBase(Arg&& urlStr) noexcept(std::is_nothrow_constructible_v<StringType, Arg>)

			: rawUrl(StringType{ std::forward<Arg>(urlStr) })
		{
			wellFormed = Parse(rawUrl, components);
		}

		template<std::convertible_to<std::string_view> Arg>
		UrlBase(Arg&& urlStr, Components const& components, bool wellFormed) noexcept(std::is_nothrow_constructible_v<StringType, Arg>)
		
			: rawUrl(StringType{ std::forward<Arg>(urlStr) })
			, components(components)
			, wellFormed(wellFormed)
		{}

		UrlBase(UrlBase const&) noexcept(NoThrow) = default;
		auto operator=(UrlBase const&) noexcept(NoThrow) -> UrlBase& = default;

		auto Reset() noexcept -> void;

		StringType rawUrl;
		Components components;
		bool wellFormed{};
	};

	class Url;
	class UrlView;

	class Url : public UrlBase<std::string> {
	public:

		friend UrlView;

		constexpr Url() noexcept = default;

		template<std::convertible_to<std::string_view> Arg>
		Url(Arg&& urlStr) : UrlBase(std::forward<Arg>(urlStr)) {}
		explicit Url(UrlView const& url);

		Url(Url const&) = default;
		auto operator=(Url const&) -> Url& = default;

		Url(Url&& other) noexcept;
		auto operator=(Url&& other) noexcept -> Url&;

		auto View() const& noexcept -> UrlView;
		auto View() const&& = delete;

		auto RawUrl() const& noexcept -> std::string const&;
		auto RawUrl() && noexcept -> std::string;

		auto swap(Url& other) noexcept -> void;
	};

	class UrlView : public UrlBase<std::string_view> {
	public:

		friend Url;

		constexpr UrlView() noexcept = default;

		template<std::convertible_to<std::string_view> Arg>
		UrlView(Arg const& urlStr) noexcept : UrlBase(urlStr) {}
		UrlView(Url const& url) noexcept;

		UrlView(UrlView const&) noexcept = default;
		auto operator=(UrlView const&) noexcept -> UrlView& = default;

		auto RawUrl() const noexcept -> std::string_view const&;

		auto swap(UrlView& other) noexcept -> void;
	};

	template<typename T>
	concept IsUrlType = IsAnyOf<std::remove_cv_t<T>, Url, UrlView>;

	class RawUrlReference {
	public:

		template<std::convertible_to<std::string_view> Arg>
		RawUrlReference(Arg const& urlStr) noexcept : rawUrl(urlStr) {}

		template<IsUrlType Arg>
		RawUrlReference(Arg const& url) noexcept : rawUrl(url.RawUrl()) {}

		RawUrlReference(RawUrlReference const&) noexcept = default;
		auto operator=(RawUrlReference const&) = delete;

		auto RawUrl() const noexcept -> std::string_view;
		operator std::string_view() const noexcept;

	private:

		std::string_view rawUrl;
	};

	auto UrlCombine(RawUrlReference baseUrl, std::filesystem::path const& path) -> Url;
}

namespace std {

	template<::Citrine::IsUrlType T, ::Citrine::IsAnyOf<char, wchar_t> CharT>
	struct formatter<T, CharT> : formatter<basic_string_view<CharT>, CharT> {

		auto format(T const& url, auto& ctx) const -> auto {

			using namespace ::Citrine;

			if constexpr (std::same_as<CharT, wchar_t>)
				return formatter<wstring_view, wchar_t>::format(ToUtf16(url.RawUrl()), ctx);
			else
				return formatter<string_view, char>::format(url.RawUrl(), ctx);
		}
	};
}

namespace glz {

	template<::Citrine::IsUrlType T>
	struct from<JSON, T>
	{
		template<auto Opts>
		static auto op(T& url, is_context auto&& ctx, auto&&... args) -> void {

			using namespace ::Citrine;

			auto str = std::string_view{};
			parse<JSON>::op<Opts>(str, ctx, args...);

			url = T{ str };
			if (!url.IsEmpty() && !url.IsWellFormed())
				ctx.error = error_code::parse_error;
		}
	};

	template<::Citrine::IsUrlType T>
	struct to<JSON, T>
	{
		template<auto Opts>
		static auto op(T const& url, auto&&... args) noexcept -> void {

			using namespace ::Citrine;

			serialize<JSON>::op<Opts>(url.RawUrl(), args...);
		}
	};
}