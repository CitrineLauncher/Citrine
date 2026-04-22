#pragma once

#include "LogMessage.h"

#include <atomic>

namespace Citrine {

	//MPSC queue for log messages
	class LogMessageQueue {
	public:

		LogMessageQueue() = default;

		LogMessageQueue(LogMessageQueue const&) = delete;
		auto operator=(LogMessageQueue const&) = delete;

		auto EnqueueMessage(LogMessage&& message) -> void;
		auto DequeueMessages(LogMessageList& outMessages) -> void;

	private:

		enum struct State {

			Empty,
			Enqueuing,
			HasData,
			Dequeuing
		};

		LogMessageList messages{};
		std::atomic<State> state{ State::Empty };
	};
}
