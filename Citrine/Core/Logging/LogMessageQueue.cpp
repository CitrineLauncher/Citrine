#include "pch.h"
#include "LogMessageQueue.h"

namespace Citrine {

	auto LogMessageQueue::EnqueueMessage(LogMessage&& message) -> void {

		while (true) {

			auto expected = State::Empty;
			if (state.compare_exchange_strong(expected, State::Enqueuing, std::memory_order::acquire, std::memory_order::relaxed)) break;

			expected = State::HasData;
			if (state.compare_exchange_strong(expected, State::Enqueuing, std::memory_order::acquire, std::memory_order::relaxed)) break;

			if (expected != State::Empty)
				state.wait(expected, std::memory_order::relaxed);
		}

		messages.Add(std::move(message));
		state.store(State::HasData, std::memory_order::release);
		state.notify_one();
	}

	auto LogMessageQueue::DequeueMessages(LogMessageList& outMessages) -> void {

		outMessages.Clear();
		while (true) {

			auto expected = State::HasData;
			if (state.compare_exchange_strong(expected, State::Dequeuing, std::memory_order::acquire, std::memory_order::relaxed)) break;
			state.wait(expected, std::memory_order::relaxed);
		}

		messages.swap(outMessages);
		state.store(State::Empty, std::memory_order::release);
		state.notify_one();
	}
}