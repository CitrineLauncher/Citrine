#pragma once

#include "MinecraftBedrockGamePackageView.g.h"
#include "MinecraftBedrockGamePackageViewBase.h"

namespace winrt::Citrine::implementation
{
    struct MinecraftBedrockGamePackageView : MinecraftBedrockGamePackageViewT<MinecraftBedrockGamePackageView>, MinecraftBedrockGamePackageViewBase
    {
        MinecraftBedrockGamePackageView();

        auto GamePackage() const -> Citrine::MinecraftBedrockGamePackageItem;
        auto GamePackage(Citrine::MinecraftBedrockGamePackageItem const& value) -> void;
        static auto GamePackageProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto InvokeActionCommand() const -> winrt::Microsoft::UI::Xaml::Input::ICommand;
        auto InvokeActionCommand(winrt::Microsoft::UI::Xaml::Input::ICommand const& value) -> void;
        static auto InvokeActionCommandProperty() noexcept -> winrt::Microsoft::UI::Xaml::DependencyProperty;

        auto OnPointerEntered(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) -> void;
        auto OnPointerExited(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) -> void;
        auto OnPointerPressed(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
        auto OnPointerReleased(winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;

        auto OnKeyDown(winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args) -> void;

        auto CancelButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto MoreOptionsButton_Click(winrt::Windows::Foundation::IInspectable const& , winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;

        auto LaunchEditorButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto RenameButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto OpenDataDirectoryButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;
        auto ManageButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;

        auto ContentRoot_DragOver(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs args) -> winrt::fire_and_forget;
        auto ContentRoot_Drop(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs args) -> winrt::fire_and_forget;

    private:

        auto EnsureProperties() -> void;
        auto OnPropertyChanged(this winrt::Microsoft::UI::Xaml::DependencyObject const& sender, winrt::Microsoft::UI::Xaml::DependencyPropertyChangedEventArgs const& args) -> void;

        auto OnGamePackagePropertyChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args) -> void;
        auto UpdateOperationVisualState() -> void;

        auto OnActivated() -> void;
        auto ExecuteInvokeActionCommand(std::wstring_view action, winrt::Windows::Foundation::IInspectable argOverride = nullptr) -> void;

        static winrt::Microsoft::UI::Xaml::DependencyProperty gamePackageProperty;
        static winrt::Microsoft::UI::Xaml::DependencyProperty invokeActionCommandProperty;

        bool pointerOver{};
        bool pointerPressed{};
        Citrine::MinecraftBedrockGamePackageItem::PropertyChanged_revoker gamePackagePropertyChangedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MinecraftBedrockGamePackageView : MinecraftBedrockGamePackageViewT<MinecraftBedrockGamePackageView, implementation::MinecraftBedrockGamePackageView>
    {
    };
}
