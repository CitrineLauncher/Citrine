#pragma once

#include "WindowEx.g.h"

#include "Core/Util/Event.h"

namespace winrt::Citrine::implementation
{
    struct WindowEx : WindowExT<WindowEx>
    {
        WindowEx();

        auto WindowContent() const -> winrt::Windows::Foundation::IInspectable;
        auto WindowContent(winrt::Windows::Foundation::IInspectable const& value) -> void;

        auto NativeHandle() const -> ::HWND;

        ~WindowEx() noexcept;

    protected:

        virtual auto OnWindowMessage(::HWND window, ::UINT messageId, ::WPARAM wParam, ::LPARAM lParam) -> void;

    private:

        auto OnActualThemeChanged(winrt::Microsoft::UI::Xaml::ElementTheme theme) -> void;
        auto UpdateCaptionButtonColors(winrt::Microsoft::UI::Xaml::ElementTheme theme) -> void;
        auto UpdateContextMenuTheme(winrt::Microsoft::UI::Xaml::ElementTheme theme) -> void;

        static auto CALLBACK SubclassProc(::HWND hWnd, ::UINT uMsg, ::WPARAM wParam, ::LPARAM lParam, ::UINT_PTR, ::DWORD_PTR dwRefData) -> ::LRESULT;

        ::HWND handle{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::ContentPresenter contentPresenter;
        winrt::Microsoft::UI::Xaml::Media::MicaBackdrop backdrop;

        ::Citrine::EventRevoker requestedThemeChangedRevoker;
        ::Citrine::EventRevoker requestedBackdropChangedRevoker;
        winrt::Microsoft::UI::Xaml::FrameworkElement::ActualThemeChanged_revoker actualThemeChangedRevoker;
    };
}

namespace winrt::Citrine::factory_implementation
{
    struct WindowEx : WindowExT<WindowEx, implementation::WindowEx>
    {
    };
}
