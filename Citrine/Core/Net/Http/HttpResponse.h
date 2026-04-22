#pragma once

#include "Common.h"
#include "HttpStatusCode.h"
#include "HttpHeader.h"

#include <optional>

#include <winrt/Windows.Storage.Streams.h>

namespace Citrine {

	class HttpResponseContent {
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

		HttpResponseContent(winrt::Windows::Storage::Streams::IBuffer&& buffer) noexcept;
		HttpResponseContent(winrt::Windows::Storage::Streams::IBuffer&& buffer, HttpHeaderCollection&& headers) noexcept;

		HttpResponseContent(HttpResponseContent const&) = delete;
		auto operator=(HttpResponseContent const&) = delete;

		HttpResponseContent(HttpResponseContent&& other) noexcept;
		auto operator=(HttpResponseContent&& other) noexcept -> HttpResponseContent&;

		auto operator[](size_type index) noexcept -> reference;
		auto operator[](size_type index) const noexcept -> const_reference;
		auto data() noexcept -> pointer;
		auto data() const noexcept -> const_pointer;

		auto begin() noexcept -> iterator;
		auto begin() const noexcept -> const_iterator;
		auto end() noexcept -> iterator;
		auto end() const noexcept -> const_pointer;

		auto empty() const noexcept -> bool;
		auto size() const noexcept -> size_type;

	private:

		std::uint8_t* myData{};
		std::size_t mySize{};
		winrt::Windows::Storage::Streams::IBuffer buffer;

	public:

		HttpHeaderCollection Headers;
	};

	template<typename StreamT, typename SizeT>
	class StreamedHttpResponseContent {
	public:

		StreamedHttpResponseContent(StreamT&& stream, SizeT size) noexcept;
		StreamedHttpResponseContent(StreamT&& stream, SizeT size, HttpHeaderCollection&& headers) noexcept;

		StreamedHttpResponseContent(StreamedHttpResponseContent const&) = delete;
		auto operator=(StreamedHttpResponseContent const&) = delete;

		StreamedHttpResponseContent(StreamedHttpResponseContent&& other) noexcept;
		auto operator=(StreamedHttpResponseContent&& other) noexcept -> StreamedHttpResponseContent&;

		auto Stream() const noexcept -> StreamT;
		auto Size() const noexcept -> SizeT;

	private:

		StreamT stream;
		SizeT size;

	public:

		HttpHeaderCollection Headers;
	};

	template<typename T>
	struct BasicHttpResponse {

		auto IsSuccessful() const noexcept -> bool;
		explicit operator bool() const noexcept;

		HttpStatusCode StatusCode{};
		T Content;
		HttpHeaderCollection Headers;
	};

	using HttpResponse = BasicHttpResponse<
		HttpResponseContent
	>;

	using HttpStreamResponse = BasicHttpResponse<
		StreamedHttpResponseContent<winrt::Windows::Storage::Streams::IInputStream, std::optional<std::uint64_t>>
	>;

	using HttpRandomAccessStreamResponse = BasicHttpResponse<
		StreamedHttpResponseContent<winrt::Windows::Storage::Streams::IRandomAccessStream, std::uint64_t>
	>;
}
