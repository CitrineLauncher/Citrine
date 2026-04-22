#include "pch.h"
#include "ApplicationMutex.h"

namespace Citrine {

    ApplicationMutex::ApplicationMutex(wchar_t const* name) noexcept 
    
        : handle(::CreateMutexW(nullptr, FALSE, name))
        , locked(handle && ::GetLastError() != ERROR_ALREADY_EXISTS)
    {}

    ApplicationMutex::ApplicationMutex(ApplicationMutex&& other) noexcept

        : handle(std::exchange(other.handle, nullptr))
        , locked(std::exchange(other.locked, false))
    {}

    auto ApplicationMutex::operator=(ApplicationMutex&& other) noexcept -> ApplicationMutex& {

        ApplicationMutex{ std::move(other) }.swap(*this);
        return *this;
    }

    auto ApplicationMutex::IsOpen() const noexcept -> bool {

        return static_cast<bool>(handle);
    }

    ApplicationMutex::operator bool() const noexcept {

        return IsOpen();
    }

    auto ApplicationMutex::IsLocked() const noexcept -> bool {

        return locked;
    }

    auto ApplicationMutex::Close() noexcept -> void {

        if (IsOpen()) {

            ::CloseHandle(handle);
            handle = nullptr;
        }
    }

    auto ApplicationMutex::swap(ApplicationMutex& other) noexcept -> void {

        std::swap(handle, other.handle);
        std::swap(locked, other.locked);
    }

    ApplicationMutex::~ApplicationMutex() {

        Close();
    }
}