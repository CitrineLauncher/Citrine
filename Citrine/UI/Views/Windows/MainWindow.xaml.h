#pragma once

#include "UI/Controls/WindowEx.h"
#include "MainWindow.g.h"

#include "UI/ViewModels/MainWindowViewModel.h"

namespace winrt::Citrine::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow() = default;

        auto InitializeComponent() -> void;

        auto ViewModel() const noexcept -> Citrine::MainWindowViewModel;

        auto ContentRoot_DragEnter(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::DragEventArgs args) -> winrt::fire_and_forget;
        auto NavView_SelectionChanged(winrt::Microsoft::UI::Xaml::Controls::NavigationView const&, winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionChangedEventArgs const& args) -> void;

    private:

        auto UpdateTitleBarDragArea() -> void;

        auto OnWindowMessage(::HWND window, ::UINT messageId, ::WPARAM wParam, ::LPARAM lParam) -> void final override;

        winrt::com_ptr<implementation::MainWindowViewModel> viewModel = winrt::make_self<implementation::MainWindowViewModel>();
        Citrine::FrameEx frame;
        ::UINT redirectEventId{};

        winrt::Microsoft::UI::Xaml::FrameworkElement::SizeChanged_revoker titleBarSizeChangedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
