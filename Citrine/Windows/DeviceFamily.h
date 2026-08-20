#pragma once

#include "Core/Util/VersionNumber.h"

namespace Citrine::Windows {

    class DeviceFamily {
    public:

        enum struct ValueType : std::uint32_t {};

        static const DeviceFamily Unknown;
        static const DeviceFamily Universal;
        static const DeviceFamily Windows8x;
        static const DeviceFamily WindowsPhone8x;
        static const DeviceFamily Desktop;
        static const DeviceFamily Mobile;
        static const DeviceFamily Xbox;
        static const DeviceFamily Team;
        static const DeviceFamily IoT;
        static const DeviceFamily IoTHeadless;
        static const DeviceFamily Server;
        static const DeviceFamily Holographic;
        static const DeviceFamily XBoxSRA;
        static const DeviceFamily XBoxERA;
        static const DeviceFamily ServerNano;
        static const DeviceFamily _8828080;
        static const DeviceFamily _7067329;
        static const DeviceFamily Core;
        static const DeviceFamily CoreHeadless;

        constexpr DeviceFamily() noexcept = default;

        explicit constexpr DeviceFamily(std::underlying_type_t<ValueType> value) noexcept

            : value{ value }
        {}

        explicit DeviceFamily(std::string_view name) noexcept;

        constexpr DeviceFamily(DeviceFamily const&) noexcept = default;
        constexpr auto operator=(DeviceFamily const&) noexcept -> DeviceFamily& = default;

        auto Name() const noexcept -> std::string_view;

        explicit operator std::string() const;

        constexpr operator ValueType() const noexcept {

            return value;
        }

        template<std::integral T>
        explicit constexpr operator T() const noexcept {

            return static_cast<T>(value);
        }

        constexpr auto operator<=>(DeviceFamily const&) const noexcept -> std::strong_ordering = default;

    private:

        ValueType value{ 0xFFFFFFFF };
    };

    inline constexpr DeviceFamily DeviceFamily::Unknown         { 0xFFFFFFFF };
    inline constexpr DeviceFamily DeviceFamily::Universal       { 0 };
    inline constexpr DeviceFamily DeviceFamily::Windows8x       { 1 };
    inline constexpr DeviceFamily DeviceFamily::WindowsPhone8x  { 2 };
    inline constexpr DeviceFamily DeviceFamily::Desktop         { 3 };
    inline constexpr DeviceFamily DeviceFamily::Mobile          { 4 };
    inline constexpr DeviceFamily DeviceFamily::Xbox            { 5 };
    inline constexpr DeviceFamily DeviceFamily::Team            { 6 };
    inline constexpr DeviceFamily DeviceFamily::IoT             { 7 };
    inline constexpr DeviceFamily DeviceFamily::IoTHeadless     { 8 };
    inline constexpr DeviceFamily DeviceFamily::Server          { 9 };
    inline constexpr DeviceFamily DeviceFamily::Holographic     { 10 };
    inline constexpr DeviceFamily DeviceFamily::XBoxSRA         { 11 };
    inline constexpr DeviceFamily DeviceFamily::XBoxERA         { 12 };
    inline constexpr DeviceFamily DeviceFamily::ServerNano      { 13 };
    inline constexpr DeviceFamily DeviceFamily::_8828080        { 14 };
    inline constexpr DeviceFamily DeviceFamily::_7067329        { 15 };
    inline constexpr DeviceFamily DeviceFamily::Core            { 16 };
    inline constexpr DeviceFamily DeviceFamily::CoreHeadless    { 17 };

    struct DeviceFamilyVersion : VersionNumberBase<DeviceFamilyVersion, 4> {

        static auto FromInteger(std::uint64_t versionNumber) noexcept -> DeviceFamilyVersion;

        constexpr DeviceFamilyVersion() noexcept = default;

        constexpr DeviceFamilyVersion(std::uint16_t major, std::uint16_t minor, std::uint16_t build, std::uint16_t revision) noexcept

            : Major(major)
            , Minor(minor)
            , Build(build)
            , Revision(revision)
        {}

        constexpr DeviceFamilyVersion(DeviceFamilyVersion const&) noexcept = default;
        constexpr auto operator=(DeviceFamilyVersion const&) noexcept -> DeviceFamilyVersion& = default;

        auto ToInteger(this DeviceFamilyVersion version) noexcept -> std::uint64_t;

        constexpr auto operator<=>(DeviceFamilyVersion const&) const noexcept -> std::strong_ordering = default;

        std::uint16_t Major{};
        std::uint16_t Minor{};
        std::uint16_t Build{};
        std::uint16_t Revision{};
    };

    struct DeviceFamilyInfo {

        static auto Get() -> DeviceFamilyInfo const&;

        DeviceFamily DeviceFamily;
        DeviceFamilyVersion DeviceFamilyVersion;
        std::string DeviceForm;
    };
}