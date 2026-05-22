#include "pch.h"
#include "MinecraftBedrockGamePackageItem.h"
#if __has_include("MinecraftBedrockGamePackageItem.g.cpp")
#include "MinecraftBedrockGamePackageItem.g.cpp"
#endif

#include "Models/MinecraftBedrockGameInfo.h"
#include "Core/Util/TrivialArray.h"

#include <tuple>
#include <algorithm>

using namespace Citrine;
using namespace Minecraft::Bedrock;

namespace winrt {

    using namespace Windows::Foundation::Collections;
    using namespace Windows::System;
    using namespace Microsoft::UI::Xaml::Media::Imaging;
}

namespace {

    auto CheckForPausableOperation(winrt::Citrine::MinecraftBedrockGamePackageStatus status) noexcept -> bool {

        using enum winrt::Citrine::MinecraftBedrockGamePackageStatus;

        return
            status == PreparingDownload ||
            status == Downloading;
    }

    auto CheckForCancellableOperation(winrt::Citrine::MinecraftBedrockGamePackageStatus status) noexcept -> bool {

        using enum winrt::Citrine::MinecraftBedrockGamePackageStatus;

        return
            status == PreparingDownload ||
            status == Downloading ||
            status == Extracting ||
            status == InstallationPaused ||
            status == InstallationFailed ||
            status == PreparingRepair ||
            status == Repairing ||
            status == RepairFailed;
    }
}

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGamePackageItem::MinecraftBedrockGamePackageItem(
        GamePackageIdentity const& id,
        winrt::hstring nameTag,
        GamePackageCompatibility compatibility,
        bool installed,
        enum struct GameCapabilities gameCapabilities,
        Citrine::MinecraftBedrockGamePackageStatus status
    )
        : id(id)
        , nameTag(std::move(nameTag))
        , compatibility(compatibility)
        , installed(installed)
        , gameCapabilities(gameCapabilities)
        , status(status)
    {
        auto& versionNumber = this->id.Version;
        auto buffer = TrivialArray<wchar_t, VersionNumberFormatter::MaxFormattedSize(versionNumber)>{};
        version = std::wstring_view{ buffer.data(), VersionNumberFormatter::FormatTo(buffer.data(), versionNumber) };

        auto& [baseline, major, minor, revision] = versionNumber;
        if (baseline == 1 && major >= 26 && major <= 99) {

            auto it = std::ranges::find(version, L'.');
            if (it != version.end())
                normalizedVersion = std::wstring_view{ it + 1, version.end() };
        }
        else {

            normalizedVersion = version;
        }
    }

    auto MinecraftBedrockGamePackageItem::Version() const noexcept -> winrt::hstring {

        return version;
    }

    auto MinecraftBedrockGamePackageItem::BuildType() const -> winrt::hstring {

        struct Strings {

            winrt::hstring Release  = L"Release";
            winrt::hstring Preview  = L"Preview";
            winrt::hstring Beta     = L"Beta";
        };
        static auto const strings = Strings{};

        using enum GameBuildType;

        auto buildType = winrt::hstring{};
        switch (id.BuildType) {
        case Release:   buildType = strings.Release;    break;
        case Preview:   buildType = strings.Preview;    break;
        case Beta:      buildType = strings.Beta;       break;
        }
        return buildType;
    }

    auto MinecraftBedrockGamePackageItem::Platform() const -> winrt::hstring {

        struct Strings {

            winrt::hstring WindowsUWP   = L"WindowsUWP";
            winrt::hstring WindowsGDK   = L"WindowsGDK";
            winrt::hstring XboxUWP      = L"XboxUWP";
        };
        static auto const strings = Strings{};

        using enum GamePlatform;
        
        auto platform = winrt::hstring{};
        switch (id.Platform) {
        case WindowsUWP:    platform = strings.WindowsUWP;  break;
        case WindowsGDK:    platform = strings.WindowsGDK;  break;
        case XboxUWP:       platform = strings.XboxUWP;     break;
        }
        return platform;
    }

    auto MinecraftBedrockGamePackageItem::Architecture() const noexcept -> winrt::ProcessorArchitecture {

        return static_cast<winrt::ProcessorArchitecture>(static_cast<std::int32_t>(id.Architecture));
    }

    auto MinecraftBedrockGamePackageItem::Origin() const -> winrt::hstring {

        struct Strings {

            winrt::hstring Meta     = L"Meta";
            winrt::hstring Import   = L"Import";
        };
        static auto const strings = Strings{};

        using enum GamePackageOrigin;

        auto origin = winrt::hstring{};
        switch (id.Origin) {
        case Meta:      origin = strings.Meta;      break;
        case Import:    origin = strings.Import;    break;
        }
        return origin;
    }

    auto MinecraftBedrockGamePackageItem::InstallId() const noexcept -> std::uint16_t {

        return id.InstallId;
    }

    auto MinecraftBedrockGamePackageItem::NormalizedVersion() const noexcept -> winrt::hstring {

        return normalizedVersion;
    }

    auto MinecraftBedrockGamePackageItem::NameTag() const noexcept -> winrt::hstring {

        return nameTag;
    }

    auto MinecraftBedrockGamePackageItem::NameTag(winrt::hstring const& value) -> void {

        if (nameTag != value) {

            nameTag = value;
            OnPropertyChanged(nameTagProperty);
        }
    }

    auto MinecraftBedrockGamePackageItem::IsSupported() const noexcept -> bool {

        return static_cast<bool>(compatibility);
    }

    auto MinecraftBedrockGamePackageItem::IsRenamable() const noexcept -> bool {

        return id.Origin == GamePackageOrigin::Import;
    }

    auto MinecraftBedrockGamePackageItem::IsInstalled() const noexcept -> bool {

        return installed;
    }

    auto MinecraftBedrockGamePackageItem::IsInstalled(bool value) -> void {

        if (installed != value) {

            installed = value;
            OnPropertyChanged(isInstalledProperty);
        }
    }

    auto MinecraftBedrockGamePackageItem::IsRegistered() const noexcept -> bool {

        return registered;
    }

    auto MinecraftBedrockGamePackageItem::IsRegistered(bool value) -> void {

        if (registered != value) {

            registered = value;
            OnPropertyChanged(isRegisteredProperty);
        }
    }

    auto MinecraftBedrockGamePackageItem::Status() const noexcept -> Citrine::MinecraftBedrockGamePackageStatus {

        return status;
    }

    auto MinecraftBedrockGamePackageItem::Status(Citrine::MinecraftBedrockGamePackageStatus value) -> void {

        if (status != value) {

            auto operationIsPausable = CheckForPausableOperation(value);
            auto operationWasPausable = CheckForPausableOperation(status);

            auto operationIsCancellable = CheckForCancellableOperation(value);
            auto operationWasCancellable = CheckForCancellableOperation(status);

            status = value;
            OnPropertyChanged(statusProperty);

            if (operationIsPausable != operationWasPausable)
                OnPropertyChanged(operationIsPausableProperty);

            if (operationIsCancellable != operationWasCancellable)
                OnPropertyChanged(operationIsCancellableProperty);

            OperationProgress({});
        }
    }

    auto MinecraftBedrockGamePackageItem::OperationProgress() const noexcept -> Citrine::ByteProgress {

        return operationProgress;
    }

    auto MinecraftBedrockGamePackageItem::OperationProgress(Citrine::ByteProgress const& value) -> void {

        if (operationProgress != value) {

            auto isIndeterminate = value.TotalBytesToProcess == 0;
            auto wasIndeterminate = operationProgress.TotalBytesToProcess == 0;

            operationProgress = value;
            OnPropertyChanged(operationProgressProperty);

            if (isIndeterminate != wasIndeterminate)
                OnPropertyChanged(operationProgressIsIndeterminateProperty);
        }
    }

    auto MinecraftBedrockGamePackageItem::OperationProgressIsIndeterminate() const noexcept -> bool {

        return operationProgress.TotalBytesToProcess == 0;
    }

    auto MinecraftBedrockGamePackageItem::OperationIsPausable() const noexcept -> bool {

        return CheckForPausableOperation(status);
    }

    auto MinecraftBedrockGamePackageItem::OperationIsCancellable() const noexcept -> bool {

        return CheckForCancellableOperation(status);
    }

    auto MinecraftBedrockGamePackageItem::GameSupportsEditor() const noexcept -> bool {

        return (gameCapabilities & GameCapabilities::Editor) == GameCapabilities::Editor;
    }

    auto MinecraftBedrockGamePackageItem::GameSupportsVibrantVisuals() const noexcept -> bool {

        return (gameCapabilities & GameCapabilities::VibrantVisuals) == GameCapabilities::VibrantVisuals;
    }

    auto MinecraftBedrockGamePackageItem::GameSupportsRayTracing() const noexcept -> bool {

        return (gameCapabilities & GameCapabilities::RayTracing) == GameCapabilities::RayTracing;
    }

    auto MinecraftBedrockGamePackageItem::GameAssociatedFileTypes() const noexcept -> winrt::IVectorView<winrt::hstring> {

        return implementation::MinecraftBedrockGameInfo::AssociatedFileTypes();
    }

    auto MinecraftBedrockGamePackageItem::Id() const noexcept -> ::Citrine::Minecraft::Bedrock::GamePackageIdentity const& {

        return id;
    }

    auto MinecraftBedrockGamePackageItem::Compatibility() const noexcept -> ::Citrine::Minecraft::Bedrock::GamePackageCompatibility {

        return compatibility;
    }

    auto MinecraftBedrockGamePackageItem::GameCapabilities() const noexcept -> ::Citrine::Minecraft::Bedrock::GameCapabilities {

        return gameCapabilities;
    }

    auto MinecraftBedrockGamePackageItem::CompareTo(MinecraftBedrockGamePackageItem const& other) const noexcept -> std::strong_ordering {

        auto getValues = [](MinecraftBedrockGamePackageItem const& item) static {

            auto& version = item.id.Version;
            auto& buildType = item.id.BuildType;
            auto& compatibility = item.compatibility;

            return std::tie(version.Baseline, version.Major, version.Minor, buildType, version.Revision, compatibility);
        };

        return getValues(*this) <=> getValues(other);
    }

    winrt::hstring const MinecraftBedrockGamePackageItem::nameTagProperty                           = L"NameTag";
    winrt::hstring const MinecraftBedrockGamePackageItem::isInstalledProperty                       = L"IsInstalled";
    winrt::hstring const MinecraftBedrockGamePackageItem::isRegisteredProperty                      = L"IsRegistered";
    winrt::hstring const MinecraftBedrockGamePackageItem::statusProperty                            = L"Status";
    winrt::hstring const MinecraftBedrockGamePackageItem::operationProgressProperty                 = L"OperationProgress";
    winrt::hstring const MinecraftBedrockGamePackageItem::operationProgressIsIndeterminateProperty  = L"OperationProgressIsIndeterminate";
    winrt::hstring const MinecraftBedrockGamePackageItem::operationIsPausableProperty               = L"OperationIsPausable";
    winrt::hstring const MinecraftBedrockGamePackageItem::operationIsCancellableProperty            = L"OperationIsCancellable";
}
