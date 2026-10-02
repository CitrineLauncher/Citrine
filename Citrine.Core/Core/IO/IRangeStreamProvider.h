#pragma once

#include <unknwnbase.h>

#include <winrt/Windows.Storage.Streams.h>

namespace Citrine {

	struct __declspec(uuid("895b1852-fad9-476c-aafe-6c92623cb54f")) IRangeStreamProvider : public ::IUnknown {
	
		virtual auto GetRangeStream(std::uint64_t offset, std::uint64_t size) -> winrt::Windows::Storage::Streams::IInputStream = 0;
	};
}