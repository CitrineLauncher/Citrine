#include "pch.h"
#include "Task.h"

#include <combaseapi.h>
#include <ctxtcall.h>

namespace {

    auto IsStaThread() noexcept -> bool {

        auto type = APTTYPE{};
        auto qualifier = APTTYPEQUALIFIER{};

        if (::CoGetApartmentType(&type, &qualifier) != S_OK)
            return false;

        if (type == APTTYPE_NA) {

            return
                qualifier == APTTYPEQUALIFIER_NA_ON_STA ||
                qualifier == APTTYPEQUALIFIER_NA_ON_MAINSTA;
        }
        return type != APTTYPE_MTA;
    }
}

namespace Citrine {

    auto TaskPromiseBase::CaptureContext() -> void {

        if (IsStaThread()) {

            auto result = ::CoGetObjectContext(IID_IContextCallback, &context);
            if (result != S_OK) std::terminate();
        }
    }

    auto TaskPromiseBase::ResumeInContext(void* address) -> void {

        auto data = ::ComCallData{ .pUserDefined = address };
        auto result = static_cast<::IContextCallback*>(context)->ContextCallback(
            [](::ComCallData* data) static -> ::HRESULT {

                std::coroutine_handle<>::from_address(data->pUserDefined)();
                return S_OK;
            },
            &data,
            IID_ICallbackWithNoReentrancyToApplicationSTA,
            5,
            nullptr
        );
        if (result != S_OK) std::terminate();
    }

    auto TaskPromiseBase::ReleaseContext() -> void {

        static_cast<::IContextCallback*>(context)->Release();
    }
}