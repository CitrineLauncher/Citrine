#include "pch.h"
#include "PEFileVersion.h"

#pragma comment(lib, "Version.lib")

#include <optional>

namespace Citrine::Windows {

	auto GetPEFileVersion(std::filesystem::path const& path, PEFileVersion& version, BasicBuffer* buffer) -> bool {

		auto localBuffer = std::optional<Buffer>{};
		if (!buffer)
			buffer = &localBuffer.emplace(::GetFileVersionInfoSizeW(path.c_str(), nullptr));

		if (buffer->capacity() == 0)
			return false;

		if (!::GetFileVersionInfoW(path.c_str(), 0, buffer->capacity(), buffer->data()))
			return false;

		auto fileInfo = static_cast<::VS_FIXEDFILEINFO*>(nullptr);
		auto fileInfoSize = ::UINT{};

		if (!::VerQueryValueW(buffer->data(), L"\\", std::out_ptr(fileInfo), &fileInfoSize))
			return false;

		version.Major = HIWORD(fileInfo->dwFileVersionMS);
		version.Minor = LOWORD(fileInfo->dwFileVersionMS);
		version.Build = HIWORD(fileInfo->dwFileVersionLS);
		version.Revision = LOWORD(fileInfo->dwFileVersionLS);
		return true;
	}
}