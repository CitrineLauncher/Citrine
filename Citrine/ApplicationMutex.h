#pragma once

namespace Citrine {

    class ApplicationMutex {
    public:

        constexpr ApplicationMutex() noexcept = default;

        explicit ApplicationMutex(wchar_t const* name) noexcept;

        ApplicationMutex(ApplicationMutex const&) = delete;
        auto operator=(ApplicationMutex const&) = delete;

        ApplicationMutex(ApplicationMutex&& other) noexcept;
        auto operator=(ApplicationMutex&& other) noexcept -> ApplicationMutex&;

        auto IsOpen() const noexcept -> bool;
        explicit operator bool() const noexcept;
        auto IsLocked() const noexcept -> bool;
        auto Close() noexcept -> void;

        auto swap(ApplicationMutex& other) noexcept -> void;

        ~ApplicationMutex();

    private:

        void* handle{ nullptr };
        bool locked{ false };
    };
}
