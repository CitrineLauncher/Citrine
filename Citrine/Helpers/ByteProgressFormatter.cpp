#include "pch.h"
#include "ByteProgressFormatter.h"
#if __has_include("ByteProgressFormatter.g.cpp")
#include "ByteProgressFormatter.g.cpp"
#endif

#include "Core/Util/FormatByteCount.h"

#include <winstring.h>
#pragma comment(lib, "windowsapp.lib")

using namespace Citrine;

namespace winrt::Citrine::implementation
{
	auto ByteProgressFormatter::Format(Citrine::ByteProgress const& progress, winrt::hstring const& ratioDelimiter) -> winrt::hstring {

		auto processed = FormatByteCount(L".2f", progress.BytesProcessed);
		if (progress.TotalBytesToProcess == 0)
			return winrt::hstring{ processed };
		auto total = FormatByteCount(L".2f", progress.TotalBytesToProcess);

		auto bufferSize = processed.size() + 1 + ratioDelimiter.size() + 1 + total.size();
		auto bufferData = static_cast<wchar_t*>(nullptr);

		auto bufferHandle = ::HSTRING_BUFFER{};
		if (::WindowsPreallocateStringBuffer(bufferSize, &bufferData, &bufferHandle) != S_OK)
			return {};

		auto out = bufferData;
		out = std::ranges::copy(processed, out).out;
		*out++ = L' ';
		out = std::ranges::copy(ratioDelimiter, out).out;
		*out++ = L' ';
		out = std::ranges::copy(total, out).out;

		auto stringHandle = ::HSTRING{};
		::WindowsPromoteStringBuffer(bufferHandle, &stringHandle);
		return winrt::hstring{ stringHandle, winrt::take_ownership_from_abi };
	}
}
