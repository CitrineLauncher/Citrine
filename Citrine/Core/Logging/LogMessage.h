#pragma once

#include "Common.h"

#include "Core/Util/DateTime.h"

#include <variant>

namespace Citrine {

	enum struct LogMessageType {

		Log,
		Flush,
		Shutdown
	};

	using LogMessagePayload = std::variant<
		std::monostate,
		std::string,
		std::wstring
	>;

	struct LogMessage {

		LogMessageType Type{};
		LogLevel Level{};
		DateTime DateTime;
		LogSourceLocation Source;
		LogMessagePayload Payload;
	};

	class LogMessageList {
	public:

		using ValueType = LogMessage;
		using SizeType = std::size_t;
		using Iterator = ValueType const*;
		using ConstIterator = ValueType const*;

		explicit LogMessageList(SizeType capacity = 128);

		LogMessageList(LogMessageList const&) = delete;
		auto operator=(LogMessageList const&) = delete;

		LogMessageList(LogMessageList&& other) noexcept;
		auto operator=(LogMessageList&& other) noexcept -> LogMessageList&;

		auto begin() const noexcept -> ConstIterator;
		auto end() const noexcept -> ConstIterator;

		auto IsEmpty() const noexcept -> bool;
		auto Size() const noexcept -> SizeType;

		auto Clear() noexcept -> void;
		auto Add(ValueType&& value) -> void;

		auto swap(LogMessageList& other) noexcept -> void;

		~LogMessageList() noexcept;

	private:

		ValueType* myData;
		SizeType mySize;
		SizeType myCapacity;
	};
}
