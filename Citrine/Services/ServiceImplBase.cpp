#include "pch.h"
#include "ServiceImplBase.h"

#include <synchapi.h>

namespace Citrine {

	auto ServiceImplBase::OperationTaskBase::CreateCompletedEvent() noexcept -> void* {

		auto event = ::CreateEventW(nullptr, true, false, nullptr);
		if (!event) std::terminate();
		return event;
	}

	auto ServiceImplBase::OperationTaskBase::SetCompletedEvent(void* event) noexcept -> void {

		::SetEvent(event);
	}

	auto ServiceImplBase::OperationTaskBase::ReleaseCompletedEvent(void* event) noexcept -> void {
		
		::CloseHandle(event);
	}
}