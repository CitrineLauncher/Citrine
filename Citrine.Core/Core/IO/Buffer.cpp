#include "pch.h"
#include "Buffer.h"

#include "Core/Util/Math.h"

#include <unknwnbase.h>
#include <robuffer.h>

using namespace Citrine;

namespace {

	struct __declspec(uuid("7cabecb1-58cc-4647-8a5c-85ec238a175b")) ISharedBufferImpl : public ::IUnknown {};

	struct SharedBufferImpl : winrt::implements<SharedBufferImpl,
		winrt::Windows::Storage::Streams::IBuffer,
		Windows::Storage::Streams::IBufferByteAccess,
		ISharedBufferImpl> {

		SharedBufferImpl(std::size_t capacity)
			
			: myData(::new std::uint8_t[capacity])
			, myCapacity(0, capacity)
		{}

		SharedBufferImpl(std::uint8_t* data, BufferCapacity capacity)

			: myData(data)
			, myCapacity(capacity)
		{}

		auto Length() const noexcept -> std::uint32_t {

			return SaturatingCast<std::uint32_t>(myCapacity.Used);
		}

		auto Length(std::uint32_t size) -> void {

			if (size > myCapacity.Total)
				throw winrt::hresult_invalid_argument{};

			myCapacity.Used = size;
		}

		auto Capacity() const noexcept -> std::uint32_t {

			return SaturatingCast<std::uint32_t>(myCapacity.Total);
		}

		auto __stdcall Buffer(std::uint8_t** value) noexcept -> ::HRESULT final override {

			*value = myData;
			return S_OK;
		}

		~SharedBufferImpl() noexcept {

			::delete[] myData;
		}

		std::uint8_t* myData{};
		BufferCapacity myCapacity{};
	};
}

namespace Citrine {

	auto BasicBuffer::operator[](size_type index) noexcept -> reference {

		return dataPtr[index];
	}

	auto BasicBuffer::operator[](size_type index) const noexcept -> const_reference {

		return dataPtr[index];
	}

	auto BasicBuffer::data() noexcept -> pointer {

		return dataPtr;
	}

	auto BasicBuffer::data() const noexcept -> const_pointer {

		return dataPtr;
	}

	auto BasicBuffer::begin() noexcept -> iterator {

		return data();
	}

	auto BasicBuffer::begin() const noexcept -> const_iterator {

		return data();
	}

	auto BasicBuffer::end() noexcept -> iterator {

		return data() + size();
	}

	auto BasicBuffer::end() const noexcept -> const_iterator {

		return data() + size();
	}

	auto BasicBuffer::empty() const noexcept -> bool {

		return size() == 0;
	}

	auto BasicBuffer::size() const noexcept -> size_type {

		return capacityPtr
			? capacityPtr->Used
			: winRtBuffer.Length();
	}

	auto BasicBuffer::resize(size_type size) -> void {

		if (capacityPtr) {

			if (size > capacityPtr->Total)
				throw std::length_error{ "Size exceeds buffer capacity" };

			capacityPtr->Used = size;
		}
		else {

			if (size > winRtBuffer.Capacity())
				throw std::length_error{ "Size exceeds buffer capacity" };

			winRtBuffer.Length(static_cast<std::uint32_t>(size));
		}
	}

	auto BasicBuffer::capacity() const noexcept -> size_type {

		return capacityPtr
			? capacityPtr->Total
			: winRtBuffer.Capacity();
	}

#pragma warning(push)
#pragma warning(disable : 26495) //Always initialize a member variable

	BasicBuffer::BasicBuffer() noexcept {

		//handle construction in derived class
	}

#pragma warning(pop)

	BasicBuffer::~BasicBuffer() noexcept {
	
		//handle destruction in derived class
	}

	Buffer::Buffer(size_type capacity) {

		dataPtr = ::new value_type[capacity];
		capacityPtr = ::new (&bufferCapacity) BufferCapacity{ 0, capacity };
	}

	Buffer::Buffer(Buffer&& other) noexcept {

		dataPtr = std::exchange(other.dataPtr, {});
		capacityPtr = ::new (&bufferCapacity) BufferCapacity{ std::exchange(other.bufferCapacity, {}) };
	}

	auto Buffer::operator=(Buffer&& other) noexcept -> Buffer& {

		Buffer{ std::move(other) }.swap(*this);
		return *this;
	}

	auto Buffer::swap(Buffer& other) noexcept -> void {

		std::swap(dataPtr, other.dataPtr);
		std::swap(bufferCapacity, other.bufferCapacity);
	}

	Buffer::~Buffer() noexcept {

		::delete[] dataPtr;
		static_assert(std::is_trivially_destructible_v<BufferCapacity>);
	}

	SharedBuffer::SharedBuffer(size_type capacity) {

		auto impl = winrt::make_self<SharedBufferImpl>(capacity);
		dataPtr = impl->myData;
		capacityPtr = &impl->myCapacity;
		::new (&winRtBuffer) IWinRTBuffer{ impl.detach()->get_abi<IWinRTBuffer>(), winrt::take_ownership_from_abi };
	}

	SharedBuffer::SharedBuffer(Buffer&& buffer) {

		auto impl = winrt::make_self<SharedBufferImpl>(std::exchange(buffer.dataPtr, {}), std::exchange(buffer.bufferCapacity, {}));
		dataPtr = impl->myData;
		capacityPtr = &impl->myCapacity;
		::new (&winRtBuffer) IWinRTBuffer{ impl.detach()->get_abi<IWinRTBuffer>(), winrt::take_ownership_from_abi };
	}

	SharedBuffer::SharedBuffer(IWinRTBuffer buffer) {

		if (buffer.try_as<ISharedBufferImpl>()) {

			auto impl = winrt::get_self<SharedBufferImpl>(buffer);
			dataPtr = impl->myData;
			capacityPtr = &impl->myCapacity;
		}
		else {

			dataPtr = buffer.data();
			capacityPtr = nullptr;
		}
		::new (&winRtBuffer) IWinRTBuffer{ std::move(buffer) };
	}

	SharedBuffer::SharedBuffer(SharedBuffer& other) noexcept {

		dataPtr = other.dataPtr;
		capacityPtr = other.capacityPtr;
		::new (&winRtBuffer) IWinRTBuffer{ other.winRtBuffer };
	}

	auto SharedBuffer::operator=(SharedBuffer& other) noexcept -> SharedBuffer& {

		dataPtr = other.dataPtr;
		capacityPtr = other.capacityPtr;
		winRtBuffer = other.winRtBuffer;
		return *this;
	}

	SharedBuffer::SharedBuffer(SharedBuffer&& other) noexcept {

		dataPtr = std::exchange(other.dataPtr, {});
		capacityPtr = std::exchange(other.capacityPtr, {});
		::new (&winRtBuffer) IWinRTBuffer{ std::move(other.winRtBuffer) };
	}

	auto SharedBuffer::operator=(SharedBuffer&& other) noexcept -> SharedBuffer& {

		dataPtr = std::exchange(other.dataPtr, {});
		capacityPtr = std::exchange(other.capacityPtr, {});
		winRtBuffer = std::move(other.winRtBuffer);
		return *this;
	}

	auto SharedBuffer::swap(SharedBuffer& other) noexcept -> void {

		std::swap(dataPtr, other.dataPtr);
		std::swap(capacityPtr, other.capacityPtr);
		std::swap(winRtBuffer, other.winRtBuffer);
	}

	SharedBuffer::operator IWinRTBuffer const& () const noexcept {

		return winRtBuffer;
	}

	SharedBuffer::~SharedBuffer() noexcept {

		winRtBuffer.~IWinRTBuffer();
	}
}