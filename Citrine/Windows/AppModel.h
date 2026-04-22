#pragma once

#include "Core/Util/Concepts.h"
#include "Core/Util/VersionNumber.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Util/Hash.h"
#include "Core/Unicode/Utf.h"

#include <string>
#include <format>
#include <algorithm>

#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>

namespace Citrine::Windows {

	struct PackageVersion : VersionNumberBase<PackageVersion, 4> {

        constexpr PackageVersion() noexcept = default;

        constexpr PackageVersion(std::uint16_t major, std::uint16_t minor, std::uint16_t build, std::uint16_t revision) noexcept

            : Major(major)
            , Minor(minor)
            , Build(build)
            , Revision(revision)
        {}

        constexpr PackageVersion(PackageVersion const&) noexcept = default;
        constexpr auto operator=(PackageVersion const&) noexcept -> PackageVersion& = default;

        constexpr auto operator<=>(PackageVersion const&) const noexcept -> std::strong_ordering = default;

        std::uint16_t Major{};
        std::uint16_t Minor{};
        std::uint16_t Build{};
        std::uint16_t Revision{};
	};

    class PackageArchitecture {
    public:

        enum struct ValueType : std::uint16_t {};

        static const PackageArchitecture Unknown;
        static const PackageArchitecture X86;
        static const PackageArchitecture Arm;
        static const PackageArchitecture X64;
        static const PackageArchitecture Neutral;
        static const PackageArchitecture Arm64;
        static const PackageArchitecture X86OnArm64;

        constexpr PackageArchitecture() noexcept = default;

        explicit constexpr PackageArchitecture(std::underlying_type_t<ValueType> value) noexcept
            
            : value{ value }
        {}

        explicit PackageArchitecture(std::string_view name) noexcept;

        constexpr PackageArchitecture(PackageArchitecture const&) noexcept = default;
        constexpr auto operator=(PackageArchitecture const&) noexcept -> PackageArchitecture& = default;

        auto Name() const noexcept -> std::string_view;

        explicit operator std::string() const;

        constexpr operator ValueType() const noexcept {

            return value;
        }

        template<std::integral T>
        explicit constexpr operator T() const noexcept {

            return static_cast<T>(value);
        }

        constexpr auto operator<=>(PackageArchitecture const&) const noexcept -> std::strong_ordering = default;

    private:

        ValueType value{ 0xFFFF };
    };

    inline constexpr PackageArchitecture PackageArchitecture::Unknown    { 0xFFFF };
    inline constexpr PackageArchitecture PackageArchitecture::X86        { 0 };
    inline constexpr PackageArchitecture PackageArchitecture::Arm        { 5 };
    inline constexpr PackageArchitecture PackageArchitecture::X64        { 9 };
    inline constexpr PackageArchitecture PackageArchitecture::Neutral    { 11 };
    inline constexpr PackageArchitecture PackageArchitecture::Arm64      { 12 };
    inline constexpr PackageArchitecture PackageArchitecture::X86OnArm64 { 14 };

    class PackageIdentityComponentBase {
    protected:

        struct Component {

            std::uint8_t Position{};
            std::uint8_t Size{};
        };

        struct Components {

            Component Name{};
            PackageVersion Version{};
            PackageArchitecture Architecture{};
            Component ResourceId{};
            Component PublisherId{};
        };

        static auto Parse(std::string_view fullName, Components& components) noexcept -> bool;
    };

    template<typename StringType>
    class PackageIdentityBase : protected PackageIdentityComponentBase {

        static constexpr auto NoThrow = std::is_nothrow_constructible_v<StringType, char const*, std::size_t>;

    public:

        auto Name() const noexcept(NoThrow) -> StringType;
        auto Version() const noexcept -> PackageVersion;
        auto Architecture() const noexcept -> PackageArchitecture;
        auto ResourceId() const noexcept(NoThrow) -> StringType;
        auto PublisherId() const noexcept(NoThrow) -> StringType;

        auto IsEmpty() const noexcept -> bool;
        auto IsValid() const noexcept -> bool;

    protected:

        constexpr PackageIdentityBase() noexcept = default;

        template<std::convertible_to<std::string_view> Arg>
        PackageIdentityBase(Arg&& fullNameStr) noexcept(std::is_nothrow_constructible_v<StringType, Arg>)

            : fullName(StringType{ std::forward<Arg>(fullNameStr) })
        {
            valid = Parse(fullName, components);
        }

        template<std::convertible_to<std::string_view> Arg>
        PackageIdentityBase(Arg&& fullNameStr, Components const& components, bool valid) noexcept(std::is_nothrow_constructible_v<StringType, Arg>)

            : fullName(StringType{ std::forward<Arg>(fullNameStr) })
            , components(components)
            , valid(valid)
        {}

        PackageIdentityBase(PackageIdentityBase const&) noexcept(NoThrow) = default;
        auto operator=(PackageIdentityBase const&) noexcept(NoThrow) -> PackageIdentityBase& = default;

        auto Reset() noexcept -> void;

        StringType fullName;
        Components components;
        bool valid{};
    };

    class PackageIdentity;
    class PackageIdentityView;

    class PackageIdentity : public PackageIdentityBase<std::string> {
    public:

        friend PackageIdentityView;

        constexpr PackageIdentity() noexcept = default;

        template<std::convertible_to<std::string_view> Arg>
        PackageIdentity(Arg&& fullNameStr) : PackageIdentityBase(std::forward<Arg>(fullNameStr)) {}
        explicit PackageIdentity(PackageIdentityView const& packageIdentity);

        PackageIdentity(PackageIdentity const&) = default;
        auto operator=(PackageIdentity const&) -> PackageIdentity& = default;

        PackageIdentity(PackageIdentity&& other) noexcept;
        auto operator=(PackageIdentity&& other) noexcept -> PackageIdentity&;

        auto View() const& noexcept -> PackageIdentityView;
        auto View() const&& = delete;

        auto FullName() const& noexcept -> std::string const&;
        auto FullName() && noexcept -> std::string;

        auto swap(PackageIdentity& other) noexcept -> void;
    };

    class PackageIdentityView : public PackageIdentityBase<std::string_view> {
    public:

        friend PackageIdentity;

        constexpr PackageIdentityView() noexcept = default;

        template<std::convertible_to<std::string_view> Arg>
        PackageIdentityView(Arg const& fullNameStr) noexcept : PackageIdentityBase(fullNameStr) {}
        PackageIdentityView(PackageIdentity const& packageIdentity) noexcept;

        PackageIdentityView(PackageIdentityView const&) noexcept = default;
        auto operator=(PackageIdentityView const&) noexcept -> PackageIdentityView& = default;

        auto FullName() const noexcept -> std::string_view const&;

        auto swap(PackageIdentityView& other) noexcept -> void;
    };

    template<typename T>
    concept IsPackageIdentityType = IsAnyOf<std::remove_cv_t<T>, PackageIdentity, PackageIdentityView>;

    auto ValidatePackageString(std::string_view str) noexcept -> bool;
    auto GetPublisherIdFromPublisher(std::string_view publisher) noexcept -> std::string;
    auto GetPackageFamilyNameFromFullName(std::string_view fullName) -> std::string;
    auto GetPackageFamilyNameFromId(PackageIdentityView id) -> std::string;
}

namespace std {

    template<::Citrine::IsAnyOf<char, wchar_t> CharT>
    struct formatter<::Citrine::Windows::PackageArchitecture, CharT> {

        constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) const -> auto {

            return ctx.begin();
        }

        auto format(::Citrine::Windows::PackageArchitecture architecture, auto& ctx) const -> auto {

            return std::ranges::copy(architecture.Name(), ctx.out()).out;
        }
    };

    template<::Citrine::Windows::IsPackageIdentityType T, ::Citrine::IsAnyOf<char, wchar_t> CharT>
    struct formatter<T, CharT> : formatter<basic_string_view<CharT>, CharT> {

        auto format(T const& packageIdentity, auto& ctx) const -> auto {

            using namespace ::Citrine;

            if constexpr (std::same_as<CharT, wchar_t>)
                return formatter<wstring_view, wchar_t>::format(ToUtf16(packageIdentity.FullName()), ctx);
            else
                return formatter<string_view, char>::format(packageIdentity.FullName(), ctx);
        }
    };

    template<>
    struct hash<::Citrine::Windows::PackageArchitecture> {

        static constexpr auto operator()(::Citrine::Windows::PackageArchitecture architecture) noexcept -> std::size_t {

            using namespace ::Citrine;
            using T = Windows::PackageArchitecture;

            return FNV1a::AppendValue(FNV1a::OffsetBasis, static_cast<T::ValueType>(architecture));
        }
    };
}

namespace glz {

    template<>
    struct from<JSON, ::Citrine::Windows::PackageArchitecture> {

        template<auto Opts>
        static auto op(::Citrine::Windows::PackageArchitecture& architecture, is_context auto&& ctx, auto&&... args) -> void {

            using namespace ::Citrine;
            using T = Windows::PackageArchitecture;

            auto json = raw_json_view{};
            parse<JSON>::op<Opts>(json, ctx, args...);

            auto begin = json.str.data();
            auto end = begin + json.str.size();

            if (end - begin >= 2 && begin[0] == '"' && end[-1] == '"') {

                auto name = std::string_view{ begin + 1, end - 1 };
                if ((architecture = T{ name }) == T::Unknown) {

                    ctx.error = error_code::unexpected_enum;
                    return;
                }
            }
            else {

                auto underlying = std::underlying_type_t<T::ValueType>{};
                auto [pos, error] = ParseInteger(begin, end, underlying);

                if (error != ParseIntegerError::None || pos != end) {

                    ctx.error = error_code::parse_error;
                    return;
                }
                architecture = T{ underlying };
            }
        }
    };

    template<>
    struct to<JSON, ::Citrine::Windows::PackageArchitecture> {

        template<auto Opts>
        static auto op(::Citrine::Windows::PackageArchitecture architecture, auto&&... args) -> void {

            using namespace ::Citrine;
            using T = Windows::PackageArchitecture;

            auto name = architecture.Name();
            if (name != "<unknown>") {

                serialize<JSON>::op<Opts>(name, args...);
            }
            else {

                auto underlying = std::to_underlying(static_cast<T::ValueType>(architecture));
                serialize<JSON>::op<Opts>(underlying, args...);
            }
        }
    };
}