#include "pch.h"
#include "AppModel.h"

#include "Core/Util/Frozen.h"
#include "Core/Util/Ascii.h"
#include "Core/Unicode/Utf.h"
#include "Core/Codec/Base32Crockford.h"

#include <botan/hash.h>

namespace Citrine::Windows {

    PackageArchitecture::PackageArchitecture(std::string_view name) noexcept {

        struct NameComparer {

            static constexpr auto operator()(std::string_view left, std::string_view right) noexcept -> bool {

                constexpr auto toLower = [](char ch) static { return Ascii::ToLower(ch); };

                return std::ranges::lexicographical_compare(left, right, {}, toLower, toLower);
            }
        };

        static constexpr auto knownArchitectures = MakeFrozenMap<std::string_view, PackageArchitecture, NameComparer>({
            
            { "x86", X86 },
            { "arm", Arm },
            { "x64", X64 },
            { "neutral", Neutral },
            { "arm64", Arm64 },
            { "x86a64", X86OnArm64 }
        });

        auto it = knownArchitectures.find(name);
        if (it != knownArchitectures.end())
            value = it->second;
    }

    auto PackageArchitecture::Name() const noexcept -> std::string_view {

        auto name = std::string_view{ "<unknown>" };
        switch (value) {
        case X86:	        name = "x86";	    break;
        case Arm:		    name = "arm";		break;
        case X64:		    name = "x64";	    break;
        case Neutral:       name = "neutral";   break;
        case Arm64:		    name = "arm64";		break;
        case X86OnArm64:    name = "x86a64";	break;
        }
        return name;
    }

    PackageArchitecture::operator std::string() const {

        return std::string{ Name() };
    }

    auto PackageIdentityComponentBase::Parse(std::string_view fullName, Components& components) noexcept -> bool {

        if (fullName.empty() || fullName.size() > 127)
            return false;

        auto const begin = fullName.data();
        auto const end = begin + fullName.size();

        auto it = begin;

        auto readSegment = [&] {

            auto first = it;
            it = std::find(it, end, '_');
            return std::string_view{ first, it };
        };

        auto readLastSegment = [&] {

            auto first = it;
            it = end;
            return std::string_view{ first, it };
        };

        // Name
        {
            auto segment = readSegment();
            if (it == end)
                return false;
            ++it;

            if (segment.size() < 3 || segment.size() > 50 || !ValidatePackageString(segment))
                return false;

            auto& name = components.Name;
            name.Size = static_cast<std::uint8_t>(segment.size());
        }

        // Version
        {
            auto segment = readSegment();
            if (it == end)
                return false;
            ++it;

            auto& version = components.Version;
            if (!PackageVersion::Parse(segment, version))
                return false;
        }

        // Architecture
        {
            auto segment = readSegment();
            if (it == end)
                return false;
            ++it;

            auto& architecture = components.Architecture;
            if ((architecture = PackageArchitecture{ segment }) == PackageArchitecture::Unknown)
                return false;
        }

        // ResourceId
        {
            auto segment = readSegment();
            if (it == end)
                return false;
            ++it;

            if (segment.size() > 30 || !(ValidatePackageString(segment) || (components.Architecture == PackageArchitecture::Neutral && segment.size() == 1 && segment.front() == '~')))
                return false;

            auto& resourceId = components.ResourceId;
            resourceId.Position = static_cast<std::uint8_t>(segment.data() - begin);
            resourceId.Size = static_cast<std::uint8_t>(segment.size());
        }

        // PublisherId
        {
            auto segment = readLastSegment();
            if (Base32Crockford::DecodedSize(segment) != 8 || !Base32Crockford::Validate(segment))
                return false;

            auto& publisherId = components.PublisherId;
            publisherId.Position = static_cast<std::uint8_t>(segment.data() - begin);
            publisherId.Size = static_cast<std::uint8_t>(segment.size());
        }

        return true;
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::Name() const noexcept(NoThrow) -> StringType {

        auto name = components.Name;
        return { fullName.data() + name.Position, name.Size };
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::Version() const noexcept -> PackageVersion {

        auto version = components.Version;
        return version;
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::Architecture() const noexcept -> PackageArchitecture {

        auto architecture = components.Architecture;
        return architecture;
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::ResourceId() const noexcept(NoThrow) -> StringType {

        auto resourceId = components.ResourceId;
        return { fullName.data() + resourceId.Position, resourceId.Size };
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::PublisherId() const noexcept(NoThrow) -> StringType {

        auto publisherId = components.PublisherId;
        return { fullName.data() + publisherId.Position, publisherId.Size };
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::IsEmpty() const noexcept -> bool {

        return fullName.empty();
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::IsValid() const noexcept -> bool {

        return valid;
    }

    template<typename StringType>
    auto PackageIdentityBase<StringType>::Reset() noexcept -> void {

        fullName = {};
        components = {};
        valid = {};
    }

    template class PackageIdentityBase<std::string>;
    template class PackageIdentityBase<std::string_view>;

    PackageIdentity::PackageIdentity(PackageIdentityView const& packageIdentity)
    
        : PackageIdentityBase(packageIdentity.fullName, packageIdentity.components, packageIdentity.valid)
    {}

    PackageIdentity::PackageIdentity(PackageIdentity&& other) noexcept
    
        : PackageIdentityBase(std::move(other.fullName), other.components, other.valid)
    {
        other.Reset();
    }

    auto PackageIdentity::operator=(PackageIdentity&& other) noexcept -> PackageIdentity& {

        if (this != &other) {

            fullName = std::move(other.fullName);
            components = other.components;
            valid = other.valid;

            other.Reset();
        }
        return *this;
    }

    auto PackageIdentity::View() const& noexcept -> PackageIdentityView {

        return *this;
    }

    auto PackageIdentity::FullName() const& noexcept -> std::string const& {

        return fullName;
    }

    auto PackageIdentity::FullName() && noexcept -> std::string {

        auto value = std::move(fullName);
        Reset();
        return value;
    }

    auto PackageIdentity::swap(PackageIdentity& other) noexcept -> void {

        std::swap(fullName, other.fullName);
        std::swap(components, other.components);
        std::swap(valid, other.valid);
    }

    PackageIdentityView::PackageIdentityView(PackageIdentity const& packageIdentity) noexcept

        : PackageIdentityBase(packageIdentity.fullName, packageIdentity.components, packageIdentity.valid)
    {}

    auto PackageIdentityView::FullName() const noexcept -> std::string_view const& {

        return fullName;
    }

    auto PackageIdentityView::swap(PackageIdentityView& other) noexcept -> void {

        std::swap(fullName, other.fullName);
        std::swap(components, other.components);
        std::swap(valid, other.valid);
    }

    auto ValidatePackageString(std::string_view str) noexcept -> bool {

        constexpr auto toU32 = [](char const* in, auto... count) static {

            auto value = std::uint32_t{};
            for (auto i = 0; i < (4, ..., count); ++i) {

                if constexpr (sizeof...(count) == 0) {

                    if (in[i] == '\0')
                        break;
                }
                value |= std::uint32_t{ Ascii::ToLower(std::bit_cast<std::uint8_t>(in[i])) } << (8 * i);
            }
            return value;
        };

        if (str.empty())
            return true;

        if (str.back() == '.')
            return false;

        auto token = std::uint32_t{};
        if (str.size() >= 3) {

            token = toU32(str.data(), 3);
            if ((token == toU32("con") || token == toU32("prn") || token == toU32("aux") || token == toU32("nul")) && (str.size() == 3 || str[3] == '.'))
                return false;
        }

        if (str.size() >= 4) {

            if ((token == toU32("com") || token == toU32("lpt")) && (str[3] >= '1' && str[3] <= '9') && (str.size() == 4 || str[4] == '.'))
                return false;

            token = toU32(str.data(), 4);
            if (token == toU32("xn--"))
                return false;
        }

        constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-">{};

        auto it = str.data();
        auto const end = it + str.size();
        auto ch = char{};

        while (true) {

            while (it < end) {

                ch = *it;
                if (!isValidChar(ch))
                    break;
                ++it;
            }

            if (it == end)
                break;

            if (ch == '.') {

                if (++it; end - it < 4 || toU32(it, 4) != toU32("xn--"))
                    continue;
            }

            return false;
        }

        return true;
    }

    auto GetPublisherIdFromPublisher(std::string_view publisher) noexcept -> std::string {

        auto utf16Encoded = ToUtf16(publisher);
        auto bytes = std::as_bytes(std::span{ utf16Encoded });

        auto hash = Botan::HashFunction::create("SHA-256");
        auto digest = std::array<std::uint8_t, 0x20>{};
        hash->update(reinterpret_cast<std::uint8_t const*>(bytes.data()), bytes.size());
        hash->final(digest);

        auto publisherId = std::string{};
        Base32Crockford::EncodeLower(std::span{ digest.data(), 8 }, publisherId);
        return publisherId;
    }

    auto GetPackageFamilyNameFromFullName(std::string_view fullName) -> std::string {

        return GetPackageFamilyNameFromId(fullName);
    }

    auto GetPackageFamilyNameFromId(PackageIdentityView id) -> std::string {

        auto name = id.Name();
        auto publisherId = id.PublisherId();

        auto familyName = std::string{};
        auto familyNameSize = name.size() + 1 + publisherId.size();

        familyName.resize_and_overwrite(familyNameSize, [&](char* data, std::size_t size) {

            auto out = data;
            out = std::ranges::copy(name, out).out;
            *out++ = '_';
            out = std::ranges::copy(publisherId, out).out;
            return size;
        });
        return familyName;
    }
}