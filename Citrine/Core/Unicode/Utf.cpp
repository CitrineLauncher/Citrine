#include "pch.h"
#include "Utf.h"

namespace Citrine {

	auto ToUtf8(std::wstring_view input) -> std::string {

		auto output = std::string{};
		ToUtf8(input, AppendTo(output));
		return output;
	}

	auto ToUtf8(std::wstring_view input, std::string& output) -> void {

		output.clear();
		ToUtf8(input, AppendTo(output));
	}

	auto ToUtf8(std::wstring_view input, AppendTo<std::string> output) -> void {

		auto const in = input.data();
		auto const inSize = input.size();

		auto const oldSize = output->size();
		auto const newSize = oldSize + ::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(inSize), nullptr, 0, nullptr, nullptr);

		if (newSize == oldSize)
			return;

		output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

			::WideCharToMultiByte(CP_UTF8, 0, in, static_cast<int>(inSize), data + oldSize, static_cast<int>(size - oldSize), nullptr, nullptr);
			return size;
		});
	}

	auto ToUtf16(std::string_view input) -> std::wstring {

		auto output = std::wstring{};
		ToUtf16(input, AppendTo(output));
		return output;
	}

	auto ToUtf16(std::string_view input, std::wstring& output) -> void {

		output.clear();
		ToUtf16(input, AppendTo(output));
	}

	auto ToUtf16(std::string_view input, AppendTo<std::wstring> output) -> void {

		auto const in = input.data();
		auto const inSize = input.size();

		auto const oldSize = output->size();
		auto const newSize = oldSize + ::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(inSize), nullptr, 0);

		if (newSize == oldSize)
			return;

		output->resize_and_overwrite(newSize, [&](wchar_t* data, std::size_t size) {

			::MultiByteToWideChar(CP_UTF8, 0, in, static_cast<int>(inSize), data + oldSize, static_cast<int>(size - oldSize));
			return size;
		});
	}
}