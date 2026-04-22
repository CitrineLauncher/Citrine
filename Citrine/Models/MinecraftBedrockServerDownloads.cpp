#include "pch.h"
#include "MinecraftBedrockServerDownloads.h"
#if __has_include("MinecraftBedrockServerDownloads.g.cpp")
#include "MinecraftBedrockServerDownloads.g.cpp"
#endif

namespace winrt {

	using namespace Windows::Foundation;
}

namespace winrt::Citrine::implementation
{
	MinecraftBedrockServerDownloads::MinecraftBedrockServerDownloads(winrt::Uri windowsPackage, winrt::Uri linuxPackage)

		: windowsPackage(std::move(windowsPackage))
		, linuxPackage(std::move(linuxPackage))
	{}

	auto MinecraftBedrockServerDownloads::WindowsPackage() const noexcept -> winrt::Uri {

		return windowsPackage;
	}

	auto MinecraftBedrockServerDownloads::LinuxPackage() const noexcept -> winrt::Uri {

		return linuxPackage;
	}
}
