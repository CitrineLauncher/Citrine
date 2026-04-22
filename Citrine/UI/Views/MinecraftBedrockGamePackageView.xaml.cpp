#include "pch.h"
#include "MinecraftBedrockGamePackageView.xaml.h"
#if __has_include("MinecraftBedrockGamePackageView.g.cpp")
#include "MinecraftBedrockGamePackageView.g.cpp"
#endif

#include "Models/ParameterPair.h"
#include "Models/MinecraftBedrockGameLaunchArgs.h"
#include "Core/Util/Scope.h"
#include "Core/Util/Ascii.h"

#include <algorithm>

namespace winrt {

    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::UI::Core;
    using namespace Windows::ApplicationModel::DataTransfer;
    using namespace Windows::Storage;
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Data;
    using namespace Microsoft::UI::Xaml::Input;
    using namespace Microsoft::UI::Input;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Microsoft::UI::Xaml::Controls::Primitives;
}

using namespace Citrine;

namespace {

    using GamePackageItem = winrt::Citrine::MinecraftBedrockGamePackageItem;
    using GamePackageItemImpl = winrt::Citrine::implementation::MinecraftBedrockGamePackageItem;
    using GamePackageStatus = winrt::Citrine::MinecraftBedrockGamePackageStatus;
    using GameLaunchArgs = winrt::Citrine::MinecraftBedrockGameLaunchArgs;
    using GameLaunchArgsImpl = winrt::Citrine::implementation::MinecraftBedrockGameLaunchArgs;

    enum struct OperationState {

        NoOperation,
        Running,
        Paused,
        Failed
    };

    auto GetOperationState(GamePackageStatus status) noexcept -> OperationState {

        using enum GamePackageStatus;
        using enum OperationState;

        auto state = OperationState{};
        switch (status) {
        case NotInstalled:              state = NoOperation;    break;
        case PreparingDownload:         state = Running;        break;
        case Downloading:               state = Running;        break;
        case Extracting:                state = Running;        break;
        case CancellingInstallation:    state = Running;        break;
        case InstallationPaused:        state = Paused;         break;
        case InstallationFailed:        state = Failed;         break;
        case Installed:                 state = NoOperation;    break;
        case PreparingRepair:           state = Running;        break;
        case Repairing:                 state = Running;        break;
        case RepairFailed:              state = Failed;         break;
        case Registering:               state = Running;        break;
        case Launching:                 state = Running;        break;
        case Uninstalling:              state = Running;        break;
        case UninstallationPending:     state = Running;        break;
        case UninstallationFailed:      state = Failed;         break;
        }
        return state;
    }
}

namespace winrt::Citrine::implementation
{
    MinecraftBedrockGamePackageView::MinecraftBedrockGamePackageView() {

        EnsureProperties();
    }

    auto MinecraftBedrockGamePackageView::GamePackage() const -> Citrine::MinecraftBedrockGamePackageItem {

        return GetValue(gamePackageProperty).try_as<Citrine::MinecraftBedrockGamePackageItem>();
    }

    auto MinecraftBedrockGamePackageView::GamePackage(Citrine::MinecraftBedrockGamePackageItem const& value) -> void {

        SetValue(gamePackageProperty, value);
    }

    auto MinecraftBedrockGamePackageView::GamePackageProperty() noexcept -> winrt::DependencyProperty {

        return gamePackageProperty;
    }

    auto MinecraftBedrockGamePackageView::InvokeActionCommand() const -> winrt::ICommand {

        return GetValue(invokeActionCommandProperty).try_as<winrt::ICommand>();
    }

    auto MinecraftBedrockGamePackageView::InvokeActionCommand(winrt::ICommand const& value) -> void {

        SetValue(invokeActionCommandProperty, value);
    }

    auto MinecraftBedrockGamePackageView::InvokeActionCommandProperty() noexcept -> winrt::DependencyProperty {

        return invokeActionCommandProperty;
    }

    auto MinecraftBedrockGamePackageView::OnPointerEntered(winrt::PointerRoutedEventArgs const&) -> void {

        pointerOver = true;
        winrt::VisualStateManager::GoToState(*this, L"PointerOver", false);
    }

    auto MinecraftBedrockGamePackageView::OnPointerExited(winrt::PointerRoutedEventArgs const&) -> void {

        pointerOver = false;
        pointerPressed = false;
        winrt::VisualStateManager::GoToState(*this, L"Normal", false);
    }

    auto MinecraftBedrockGamePackageView::OnPointerPressed(winrt::PointerRoutedEventArgs const& args) -> void {

        auto updateKind = args.GetCurrentPoint(*this).Properties().PointerUpdateKind();
        if (updateKind != winrt::PointerUpdateKind::LeftButtonPressed)
            return;

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        if (operationState == Running && !gamePackageImpl->OperationIsPausable())
            return;

        pointerPressed = true;
        winrt::VisualStateManager::GoToState(*this, L"Pressed", false);
    }

    auto MinecraftBedrockGamePackageView::OnPointerReleased(winrt::PointerRoutedEventArgs const& args) -> void {

        if (!pointerPressed)
            return;

        pointerPressed = false;
        winrt::VisualStateManager::GoToState(*this, L"PointerOver", false);
        OnActivated();
    }

    auto MinecraftBedrockGamePackageView::OnKeyDown(winrt::KeyRoutedEventArgs const& args) -> void {

        if (args.Key() == winrt::VirtualKey::Enter)
            OnActivated();
    }

    auto MinecraftBedrockGamePackageView::CancelButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        if (!gamePackageImpl->OperationIsCancellable())
            return;

        ExecuteInvokeActionCommand(L"CancelOperation");
    }

    auto MinecraftBedrockGamePackageView::MoreOptionsButton_Click(winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> void {

        winrt::FlyoutBase::ShowAttachedFlyout(MoreOptionsButton());
    }

    auto MinecraftBedrockGamePackageView::LaunchEditorButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        if (operationState != NoOperation || !gamePackageImpl->IsInstalled() || !gamePackageImpl->GameSupportsEditor())
            return;

        auto launchArgs = winrt::make_self<GameLaunchArgsImpl>(gamePackage);
        launchArgs->ActivateEditor(true);

        ExecuteInvokeActionCommand(L"Launch", { launchArgs.detach()->get_abi<GameLaunchArgs>(), winrt::take_ownership_from_abi });
    }

    auto MinecraftBedrockGamePackageView::RenameButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        if (operationState != NoOperation || !gamePackageImpl->IsRenamable())
            return;

        ExecuteInvokeActionCommand(L"Rename");
    }

    auto MinecraftBedrockGamePackageView::OpenDataDirectoryButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        if (operationState != NoOperation)
            return;

        ExecuteInvokeActionCommand(L"OpenGameDataDirectory");
    }

    auto MinecraftBedrockGamePackageView::ManageButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        if (operationState != NoOperation)
            return;

        ExecuteInvokeActionCommand(L"Manage");
    }

    auto MinecraftBedrockGamePackageView::ContentRoot_DragOver(winrt::IInspectable const&, winrt::DragEventArgs args) -> winrt::fire_and_forget {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            co_return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        auto dataView = args.DataView();
        if (!dataView.Contains(winrt::StandardDataFormats::StorageItems()))
            co_return;

        auto defferal = args.GetDeferral();
        auto completeDefferal = ScopeExit{ [&] { defferal.Complete(); } };
        args.AcceptedOperation(winrt::DataPackageOperation::None);

        auto strongSelf = get_strong();
        auto storageItems = co_await dataView.GetStorageItemsAsync();

        if (operationState != NoOperation || !gamePackageImpl->IsInstalled())
            co_return;

        if (storageItems.Size() == 1) {

            auto storageFile = storageItems.GetAt(0).try_as<winrt::StorageFile>();
            if (!storageFile)
                co_return;

            auto fileType = storageFile.FileType();
            auto associatedFileTypes = gamePackageImpl->GameAssociatedFileTypes();

            for (auto associatedFileType : associatedFileTypes) {

                constexpr auto toLower = [](wchar_t ch) static { return Ascii::ToLower(ch); };
                if (std::ranges::equal(associatedFileType, fileType, {}, toLower, toLower)) {

                    args.AcceptedOperation(winrt::DataPackageOperation::Copy);
                    break;
                }
            }
        }
    }

    auto MinecraftBedrockGamePackageView::ContentRoot_Drop(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs args) -> winrt::fire_and_forget {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            co_return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        auto dataView = args.DataView();
        if (!dataView.Contains(winrt::StandardDataFormats::StorageItems()))
            co_return;

        auto strongSelf = get_strong();
        auto storageItems = co_await dataView.GetStorageItemsAsync();

        if (operationState != NoOperation || !gamePackageImpl->IsInstalled())
            co_return;

        if (storageItems.Size() == 1) {

            auto storageFile = storageItems.GetAt(0).try_as<winrt::StorageFile>();
            if (!storageFile)
                co_return;

            auto fileType = storageFile.FileType();
            auto associatedFileTypes = gamePackageImpl->GameAssociatedFileTypes();

            for (auto associatedFileType : associatedFileTypes) {

                constexpr auto toLower = [](wchar_t ch) static { return Ascii::ToLower(ch); };
                if (std::ranges::equal(associatedFileType, fileType, {}, toLower, toLower)) {

                    auto launchArgs = winrt::make_self<GameLaunchArgsImpl>(gamePackage);
                    launchArgs->FileToImport(storageFile.Path());

                    ExecuteInvokeActionCommand(L"Launch", { launchArgs.detach()->get_abi<GameLaunchArgs>(), winrt::take_ownership_from_abi });
                    break;
                }
            }
        }
    }

    auto MinecraftBedrockGamePackageView::EnsureProperties() -> void {

        if (!gamePackageProperty) {

            gamePackageProperty = winrt::DependencyProperty::Register(
                L"GamePackageProperty",
                winrt::xaml_typename<Citrine::MinecraftBedrockGamePackageItem>(),
                winrt::xaml_typename<class_type>(),
                winrt::PropertyMetadata{ nullptr, &MinecraftBedrockGamePackageView::OnPropertyChanged }
            );
        }

        if (!invokeActionCommandProperty) {

            invokeActionCommandProperty = winrt::DependencyProperty::Register(
                L"InvokeActionCommandProperty",
                winrt::xaml_typename<winrt::ICommand>(),
                winrt::xaml_typename<class_type>(),
                nullptr
            );
        }
    }

    auto MinecraftBedrockGamePackageView::OnPropertyChanged(this winrt::DependencyObject const& sender, winrt::DependencyPropertyChangedEventArgs const& args) -> void {

        auto self = sender.try_as<MinecraftBedrockGamePackageView>();
        if (!self)
            return;

        if (args.Property() == gamePackageProperty) {

            self->gamePackagePropertyChangedRevoker.revoke();
            self->UpdateOperationVisualState();

            auto gamePackage = args.NewValue().try_as<GamePackageItem>();
            if (!gamePackage)
                return;

            self->gamePackagePropertyChangedRevoker = gamePackage.PropertyChanged(winrt::auto_revoke, { self.get(), &MinecraftBedrockGamePackageView::OnGamePackagePropertyChanged });
        }
    }

    auto MinecraftBedrockGamePackageView::OnGamePackagePropertyChanged(winrt::IInspectable const&, winrt::PropertyChangedEventArgs const& args) -> void {

        auto propertyName = args.PropertyName();
        if (propertyName == L"Status") {

            UpdateOperationVisualState();
        }
    }

    auto MinecraftBedrockGamePackageView::UpdateOperationVisualState() -> void {

        using enum OperationState;
        auto [operationState, isInstalled] = [&] -> std::pair<OperationState, bool> {

            auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
            if (!gamePackage)
                return { OperationState::NoOperation, false };

            auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
            return { GetOperationState(gamePackageImpl->Status()), gamePackageImpl->IsInstalled() };
        }();

        auto visualState = std::wstring_view{ L"NoOperation" };
        switch (operationState) {
        case Running:   visualState = L"OperationRunning";  break;
        case Paused:    visualState = L"OperationPaused";   break;
        case Failed:    visualState = L"OperationFailed";   break;
        }

        if (operationState != NoOperation) {

            UnloadObject(MoreOptionsButton());
            FindName(L"StatusArea");
        }
        else {

            UnloadObject(StatusArea());
            if (isInstalled)
                FindName(L"MoreOptionsButton");
            else
                UnloadObject(MoreOptionsButton());
        }

        winrt::VisualStateManager::GoToState(*this, visualState, false);
    }

    auto MinecraftBedrockGamePackageView::OnActivated() -> void {

        auto gamePackage = GetValue(gamePackageProperty).try_as<GamePackageItem>();
        if (!gamePackage)
            return;

        auto gamePackageImpl = winrt::get_self<GamePackageItemImpl>(gamePackage);
        auto operationState = GetOperationState(gamePackageImpl->Status());
        using enum OperationState;

        auto action = std::wstring_view{};
        auto argOverride = winrt::IInspectable{ nullptr };

        if (operationState == Running) {

            if (!gamePackageImpl->OperationIsPausable())
                return;
            action = L"PauseOperation";
        }
        else if (operationState == Paused || operationState == Failed) {

            action = L"ResumeOperation";
        }
        else if (gamePackageImpl->IsInstalled()) {

            using enum winrt::CoreVirtualKeyStates;
            auto states = winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Shift);

            auto launchArgs = winrt::make_self<GameLaunchArgsImpl>(gamePackage);
            launchArgs->ActivateEditor((states & Down) == Down && gamePackageImpl->GameSupportsEditor());

            action = L"Launch";
            argOverride = { launchArgs.detach()->get_abi<GameLaunchArgs>(), winrt::take_ownership_from_abi };
        }
        else {

            action = L"Install";
        }

        ExecuteInvokeActionCommand(action, std::move(argOverride));
    }

    auto MinecraftBedrockGamePackageView::ExecuteInvokeActionCommand(std::wstring_view action, winrt::IInspectable argOverride) -> void {

        auto parameter = winrt::make<implementation::ParameterPair>(winrt::box_value(action), argOverride ? std::move(argOverride) : GetValue(gamePackageProperty));
        auto invokeActionCommand = GetValue(invokeActionCommandProperty).try_as<winrt::ICommand>();

        if (!invokeActionCommand || !invokeActionCommand.CanExecute(parameter))
            return;

        invokeActionCommand.Execute(parameter);
    }

    winrt::DependencyProperty MinecraftBedrockGamePackageView::gamePackageProperty{ nullptr };
    winrt::DependencyProperty MinecraftBedrockGamePackageView::invokeActionCommandProperty{ nullptr };
}
