#pragma once

#include "Common.h"
#include "MsixApplication.h"
#include "MsixCustomInstallExtension.h"

#include "Core/IO/File.h"
#include "Windows/AppModel.h"

#include <memory>
#include <span>
#include <type_traits>

#include <winrt/Windows.Foundation.h>

namespace Citrine::Windows {

	class MsixManifest {
	public:

		constexpr MsixManifest(std::nullptr_t) noexcept {};
		auto operator=(std::nullptr_t) noexcept -> MsixManifest&;

		MsixManifest(MsixManifest const&) = delete;
		auto operator=(MsixManifest const&) = delete;

		MsixManifest(MsixManifest&&) noexcept = default;
		auto operator=(MsixManifest&&) noexcept -> MsixManifest& = default;

		static auto OpenFromFile(File&& file) -> MsixOperationResult<MsixManifest>;

		template<typename T> requires std::is_rvalue_reference_v<T&&>
		static auto OpenFromBuffer(T&& buffer) -> MsixOperationResult<MsixManifest> {

			auto span = MakeBufferSpan(buffer);
			return OpenFromBuffer(MakeBufferHandle(std::move(buffer)), span);
		}

		auto Identity() const noexcept -> PackageIdentity const&;
		auto Application() const noexcept -> MsixApplication const&;
		auto CustomInstallExtension() const noexcept -> MsixCustomInstallExtension const&;

		operator bool() const noexcept;
		auto Release() noexcept -> void;

		auto File() && noexcept -> File;

	private:

		using BufferHandle = std::unique_ptr<void, auto(*)(void*) -> void>;

		template<typename T>
		static auto MakeBufferSpan(T& buffer) noexcept -> std::span<std::uint8_t> {

			if constexpr (std::derived_from<T, winrt::Windows::Foundation::IUnknown>)
				return { reinterpret_cast<std::uint8_t*>(buffer.data()), buffer.Length() };
			else
				return { reinterpret_cast<std::uint8_t*>(buffer.data()), buffer.size() };
		}

		template<typename T> requires std::is_rvalue_reference_v<T&&>
		static auto MakeBufferHandle(T&& buffer) noexcept -> BufferHandle {

			if constexpr (std::derived_from<T, winrt::Windows::Foundation::IUnknown>)
				return { winrt::detach_abi(buffer), +[](void* ptr) static { reinterpret_cast<::IUnknown*>(ptr)->Release(); } };
			else
				return { new T{ std::move(buffer) }, +[](void* ptr) static { delete static_cast<T*>(ptr); } };
		}

		static auto OpenFromBuffer(BufferHandle&& buffer, std::span<std::uint8_t> bufferSpan) -> MsixOperationResult<MsixManifest>;

		class Impl;

		struct ImplDeleter {

			auto operator()(Impl* impl) const -> void;
		};

		MsixManifest(std::unique_ptr<Impl, ImplDeleter>&& impl) noexcept;

		std::unique_ptr<Impl, ImplDeleter> impl{ nullptr };
	};
}
