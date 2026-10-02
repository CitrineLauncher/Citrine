#include "pch.h"
#include "HttpResponse.h"

namespace winrt {

	using namespace winrt::Windows::Storage::Streams;
}

namespace Citrine {

	HttpResponseContent::HttpResponseContent(winrt::IBuffer&& buffer) noexcept 

		: myData(buffer.data())
		, mySize(buffer.Length())
		, buffer(std::move(buffer))
	{}

	HttpResponseContent::HttpResponseContent(winrt::IBuffer&& buffer, HttpHeaderCollection&& headers) noexcept

		: myData(buffer.data())
		, mySize(buffer.Length())
		, buffer(std::move(buffer))
		, Headers(headers)
	{}

	HttpResponseContent::HttpResponseContent(HttpResponseContent&& other) noexcept

		: myData(std::exchange(other.myData, {}))
		, mySize(std::exchange(other.mySize, {}))
		, buffer(std::move(other.buffer))
		, Headers(std::move(other.Headers))
	{}

	auto HttpResponseContent::operator=(HttpResponseContent&& other) noexcept -> HttpResponseContent& {

		myData = std::exchange(other.myData, {});
		mySize = std::exchange(other.mySize, {});
		buffer = std::move(other.buffer);
		Headers = std::move(other.Headers);

		return *this;
	}

	auto HttpResponseContent::operator[](size_type index) noexcept -> reference {

		return myData[index];
	}

	auto HttpResponseContent::operator[](size_type index) const noexcept -> const_reference {

		return myData[index];
	}

	auto HttpResponseContent::data() noexcept -> pointer {

		return myData;
	}

	auto HttpResponseContent::data() const noexcept -> const_pointer {

		return myData;
	}

	auto HttpResponseContent::begin() noexcept -> iterator {

		return myData;
	}

	auto HttpResponseContent::begin() const noexcept -> const_iterator {

		return myData;
	}

	auto HttpResponseContent::end() noexcept -> iterator {

		return myData + mySize;
	}

	auto HttpResponseContent::end() const noexcept -> const_pointer {

		return myData + mySize;
	}

	auto HttpResponseContent::empty() const noexcept -> bool {

		return mySize == 0;
	}

	auto HttpResponseContent::size() const noexcept -> size_type {

		return mySize;
	}

	template<typename StreamT, typename SizeT>
	StreamedHttpResponseContent<StreamT, SizeT>::StreamedHttpResponseContent(StreamT&& stream, SizeT size) noexcept

		: stream(std::move(stream))
		, size(size)
	{}

	template<typename StreamT, typename SizeT>
	StreamedHttpResponseContent<StreamT, SizeT>::StreamedHttpResponseContent(StreamT&& stream, SizeT size, HttpHeaderCollection&& headers) noexcept

		: stream(std::move(stream))
		, size(size)
		, Headers(std::move(headers))
	{}

	template<typename StreamT, typename SizeT>
	StreamedHttpResponseContent<StreamT, SizeT>::StreamedHttpResponseContent(StreamedHttpResponseContent&& other) noexcept
	
		: stream(std::move(other.stream))
		, size(std::exchange(other.size, {}))
		, Headers(std::move(other.Headers))
	{}

	template<typename StreamT, typename SizeT>
	auto StreamedHttpResponseContent<StreamT, SizeT>::operator=(StreamedHttpResponseContent&& other) noexcept -> StreamedHttpResponseContent& {

		stream = std::move(other.stream);
		size = std::exchange(other.size, {});
		Headers = std::move(other.Headers);

		return *this;
	}

	template<typename StreamT, typename SizeT>
	auto StreamedHttpResponseContent<StreamT, SizeT>::Stream() const noexcept -> StreamT {

		return stream;
	}

	template<typename StreamT, typename SizeT>
	auto StreamedHttpResponseContent<StreamT, SizeT>::Size() const noexcept -> SizeT {

		return size;
	}

	template class StreamedHttpResponseContent<winrt::IInputStream, std::optional<std::uint64_t>>;
	template class StreamedHttpResponseContent<winrt::IRandomAccessStream, std::uint64_t>;

	template<typename T>
	auto BasicHttpResponse<T>::IsSuccessful() const noexcept -> bool {

		return StatusCode >= HttpStatusCode{ 200 } && StatusCode <= HttpStatusCode{ 299 };
	}

	template<typename T>
	BasicHttpResponse<T>::operator bool() const noexcept {

		return IsSuccessful();
	}

	template struct BasicHttpResponse<HttpResponseContent>;
	template struct BasicHttpResponse<StreamedHttpResponseContent<winrt::IInputStream, std::optional<std::uint64_t>>>;
	template struct BasicHttpResponse<StreamedHttpResponseContent<winrt::IRandomAccessStream, std::uint64_t>>;
}