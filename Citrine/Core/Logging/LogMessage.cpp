#include "pch.h"
#include "LogMessage.h"

namespace Citrine {

	LogMessageList::LogMessageList(SizeType capacity)

		: myData(new ValueType[capacity])
		, mySize(0)
		, myCapacity(capacity)
	{}

	LogMessageList::LogMessageList(LogMessageList&& other) noexcept

		: myData(std::exchange(other.myData, {}))
		, mySize(std::exchange(other.mySize, {}))
		, myCapacity(std::exchange(other.myCapacity, {}))
	{}

	auto LogMessageList::operator=(LogMessageList&& other) noexcept -> LogMessageList& {

		if (this != &other) {

			delete[] myData;
			myData = std::exchange(other.myData, {});
			mySize = std::exchange(other.mySize, {});
			myCapacity = std::exchange(other.myCapacity, {});
		}
		return *this;
	}

	auto LogMessageList::begin() const noexcept -> ConstIterator {

		return myData;
	}

	auto LogMessageList::end() const noexcept -> ConstIterator {

		return myData + mySize;
	}

	auto LogMessageList::IsEmpty() const noexcept -> bool {

		return mySize == 0;
	}

	auto LogMessageList::Size() const noexcept -> SizeType {

		return mySize;
	}

	auto LogMessageList::Clear() noexcept -> void {

		auto it = myData;
		auto const end = it + mySize;

		for (; it < end; ++it)
			it->Payload = std::monostate{};
		mySize = 0;
	}

	auto LogMessageList::Add(ValueType&& value) -> void {

		if (mySize == myCapacity) {

			auto newCapacity = std::max(myCapacity + myCapacity / 2, 16uz);
			auto newData = new ValueType[newCapacity];
			std::move(myData, myData + mySize, newData);
			delete[] myData;

			myData = newData;
			myCapacity = newCapacity;
		}

		myData[mySize++] = std::move(value);
	}

	auto LogMessageList::swap(LogMessageList& other) noexcept -> void {

		std::swap(myData, other.myData);
		std::swap(mySize, other.mySize);
		std::swap(myCapacity, other.myCapacity);
	}

	LogMessageList::~LogMessageList() noexcept {

		delete[] myData;
	}
}