#pragma once

#include <string>
#include <concepts>

namespace Citrine {

	class Unicode {
	public:

		static auto FoldCase(std::wstring_view input) -> std::wstring;
		static auto FoldCase(std::wstring_view input, std::wstring& output) -> void;
		static auto FoldCaseInPlace(std::wstring& str) noexcept -> void;

		static auto IsWhitespace(wchar_t ch) noexcept -> bool;
		static auto FindFirstWhitespace(std::wstring_view input, std::size_t offset = 0) noexcept -> std::size_t;
		static auto FindFirstNonWhitespace(std::wstring_view input, std::size_t offset = 0) noexcept -> std::size_t;
		static auto FindLastWhitespace(std::wstring_view input, std::size_t offset = std::wstring_view::npos) noexcept -> std::size_t;
		static auto FindLastNonWhitespace(std::wstring_view input, std::size_t offset = std::wstring_view::npos) noexcept -> std::size_t;

		static auto NormalizeWhitespace(std::wstring_view input) -> std::wstring;
		static auto NormalizeWhitespace(std::wstring_view input, std::wstring& output) -> void;
		static auto NormalizeWhitespaceInPlace(std::wstring& str) noexcept -> void;
	};
}
