#pragma once

#include <stdexcept>

#include <winrt/Windows.Storage.Streams.h>

namespace Citrine {

	struct BufferCapacity {

		std::size_t Used{};
		std::size_t Total{};
	};

	class BasicBuffer {
	public:

		using value_type = std::uint8_t;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using iterator = pointer;
		using const_iterator = const_pointer;

		auto operator[](size_type index) noexcept -> reference;
		auto operator[](size_type index) const noexcept -> const_reference;
		auto data() noexcept -> pointer;
		auto data() const noexcept -> const_pointer;

		auto begin() noexcept -> iterator;
		auto begin() const noexcept -> const_iterator;
		auto end() noexcept -> iterator;
		auto end() const noexcept -> const_iterator;

		auto empty() const noexcept -> bool;
		auto size() const noexcept -> size_type;

		auto resize(size_type size) -> void;
		auto capacity() const noexcept -> size_type;

	protected:

		BasicBuffer() noexcept;

		BasicBuffer(BasicBuffer const&) = delete;
		auto operator=(BasicBuffer const&) = delete;

		~BasicBuffer() noexcept;

		using IWinRTBuffer = winrt::Windows::Storage::Streams::IBuffer;

		value_type* dataPtr;
		BufferCapacity* capacityPtr;
		union {

			BufferCapacity bufferCapacity;
			IWinRTBuffer winRtBuffer;
		};
	};

	class Buffer : public BasicBuffer {
	public:

		explicit Buffer(size_type capacity);

		Buffer(Buffer&& other) noexcept;
		auto operator=(Buffer&& other) noexcept -> Buffer&;

		auto swap(Buffer& other) noexcept -> void;

		~Buffer() noexcept;

		friend class SharedBuffer;
	};

	class SharedBuffer : public BasicBuffer {
	public:

		explicit SharedBuffer(size_type capacity);
		explicit SharedBuffer(Buffer&& buffer);
		SharedBuffer(IWinRTBuffer buffer);

		SharedBuffer(SharedBuffer& other) noexcept;
		auto operator=(SharedBuffer& other) noexcept -> SharedBuffer&;

		SharedBuffer(SharedBuffer&& other) noexcept;
		auto operator=(SharedBuffer&& other) noexcept -> SharedBuffer&;

		auto swap(SharedBuffer& other) noexcept -> void;

		operator IWinRTBuffer const& () const noexcept;

		~SharedBuffer() noexcept;
	};
}