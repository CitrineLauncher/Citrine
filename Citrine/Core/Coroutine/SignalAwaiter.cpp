#include "pch.h"
#include "SignalAwaiter.h"

#include <threadpoolapiset.h>
#include <processthreadsapi.h>

#include <bit>

namespace Citrine {

	auto SignalAwaiter::await_ready() const noexcept -> bool {

		return ::WaitForSingleObject(handle, 0) == 0;
	}

	auto SignalAwaiter::CreateThreadPoolWait() -> void* {

		return ::CreateThreadpoolWait([](auto... args) static { Callback(args...); }, this, nullptr);
	}

	auto SignalAwaiter::SuspendInternal() -> void {

		auto relativeCount = -timeout.count();
		auto fileTime = std::bit_cast<::FILETIME>(relativeCount);
		::SetThreadpoolWait(static_cast<::PTP_WAIT>(waitHandle.get()), handle, relativeCount != 0 ? &fileTime : nullptr);
	}

	auto SignalAwaiter::CancelInternal() -> bool {

		return ::SetThreadpoolWaitEx(static_cast<::PTP_WAIT>(waitHandle.get()), nullptr, nullptr, nullptr);
	}

	auto SignalAwaiter::FireImmediately() -> void {

		auto now = ::FILETIME{};
		::SetThreadpoolWait(static_cast<::PTP_WAIT>(waitHandle.get()), ::GetCurrentProcess(), &now);
	}

	auto SignalAwaiter::WaitDeleter::operator()(void* handle) noexcept -> void {

		if (handle)
			::CloseThreadpoolWait(static_cast<::PTP_WAIT>(handle));
	}
}