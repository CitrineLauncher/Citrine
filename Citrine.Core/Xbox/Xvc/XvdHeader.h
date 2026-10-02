#pragma once

#include <cstdint>
#include <bit>

#include "Core/Util/Guid.h"

namespace Citrine::Xbox {

    inline constexpr auto XvdMagic = std::byteswap<std::uint64_t>(0x6d7366742d787664); // msft-xvd

    enum struct XvdVolumeFlags : std::uint32_t {

        ReadOnly = 1,
        EncryptionDisabled = 2,
        DataIntegrityDisabled = 4,
        LegacySectorSize = 8,
        ResiliencyEnabled = 16,
        SraReadOnly = 32,
        RegionIdInXts = 64,
        EraSpecific = 128
    };

    constexpr auto operator|(XvdVolumeFlags left, XvdVolumeFlags right) noexcept -> XvdVolumeFlags {

        return XvdVolumeFlags{ std::to_underlying(left) | std::to_underlying(right) };
    }

    constexpr auto operator&(XvdVolumeFlags left, XvdVolumeFlags right) noexcept -> XvdVolumeFlags {

        return XvdVolumeFlags{ std::to_underlying(left) & std::to_underlying(right) };
    }

    enum struct XvdType : std::uint32_t {

        Fixed = 0,
        Dynamic = 1
    };

    enum struct XvdContentType : std::uint32_t {

        Data = 0,
        Title = 1,
        SystemOS = 2,
        EraOS = 3,
        Scratch = 4,
        ResetData = 5,
        App = 6,
        HostOS = 7,
        Xbox360STFS = 8,
        Xbox360FATX = 9,
        Xbox360GDFX = 10,
        Updater = 11,
        OfflineUpdater = 12,
        Template = 13,
        MteHost = 14,
        MteApp = 15,
        MteTitle = 16,
        MteEraOS = 17,
        EraTools = 18,
        SystemTools = 19,
        SystemAuxiliary = 20,
        AcousticModel = 21,
        Codec = 22,
        Qaslt = 23,
        AppDlc = 24,
        TitleDlc = 25,
        UniversalDlc = 26,
        SystemData = 27,
        Test = 28,
        HwTest = 29,
        KioskData = 30,
        DevPackage = 31,
        HostProfiler = 32,
        Roamable = 33,
        ThinProvisioned = 34,
        StreamingOnlySra = 35,
        StreamingOnlyEra = 36,
        StreamingOnlyHost = 37,
        QuickResume = 38,
        HostData = 39
    };

    enum struct XvdPlatform : std::uint32_t {

        Xbox = 0,
        PC = 1,
        Gen8GameCore = 2,
        ScarlettGameCore = 4
    };

#pragma pack(push, 1)

    struct alignas(4) XvdExtEntry {

        std::uint32_t Code;
        std::uint32_t Length;
        std::uint64_t Offset;
        std::uint32_t DataLength;
        std::uint32_t Reserved;
    };

    struct alignas(4) XvdHeader {

        std::uint8_t Signature[0x200];
        std::uint64_t Cookie;
        XvdVolumeFlags VolumeFlags;
        std::uint32_t FormatVersion;
        std::int64_t CreationTime;
        std::uint64_t DriveSize;
        Guid VDUID;
        Guid UDUID;
        std::uint8_t RootHash[0x20];
        std::uint8_t XvcHash[0x20];
        XvdType Type;
        XvdContentType ContentType;
        std::uint32_t EmbeddedXvdLength;
        std::uint32_t UserDataLength;
        std::uint32_t XvcDataLength;
        std::uint32_t DynamicHeaderLength;
        std::uint32_t BlockSize;
        XvdExtEntry ExtEntries[0x4];
        std::uint16_t Capabilities[0x8];
        std::uint8_t PECatalogHash[0x20];
        std::uint8_t EmbeddedXvdPDUID[0x10];
        std::uint8_t Reserved0[0x10];
        std::uint8_t KeyMaterial[0x20];
        std::uint8_t UserDataHash[0x20];
        char SandboxId[0x10];
        Guid ProductId;
        Guid PDUID;
        std::uint64_t PackageVersionNumber;
        std::uint16_t PECatalogCaps[0x10];
        std::uint8_t PECatalogs[0x80];
        std::uint32_t WriteableExpirationDate;
        std::uint32_t WriteablePolicyFlags;
        std::uint32_t PersistentLocalStorageSize;
        std::uint8_t MutableXvcDataPageCount;
        XvdPlatform Platform;
        std::uint8_t UnusedSpace0[0x1B];
        std::int64_t SequenceNumber;
        std::uint64_t MinSystemVersion;
        std::uint32_t OdkKeyslotId;
        std::uint8_t RoamingHeader[0x800];
        std::uint8_t Reserved1[0x350];
        std::uint64_t ResilientDataOffset;
        std::uint32_t ResilientDataLength;
    };

#pragma pack(pop)
}