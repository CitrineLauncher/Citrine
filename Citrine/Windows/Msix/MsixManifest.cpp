#include "pch.h"
#include "MsixManifest.h"

#include <algorithm>

#include <pugixml.hpp>

namespace Citrine::Windows {

	class MsixManifest::Impl {
	public:

		Impl(Impl const&) = delete;
		auto operator=(Impl const&) = delete;

		static auto OpenFromFile(Citrine::File&& file) -> MsixOperationResult<MsixManifest> {

			auto impl = std::unique_ptr<Impl, ImplDeleter>{ new Impl{ std::move(file) } };
			auto result = impl->Initialize();

			if (!result)
				return result.error();
			return MsixManifest{ std::move(impl) };
		}

		static auto OpenFromBuffer(BufferHandle&& buffer, std::span<std::uint8_t> bufferSpan) -> MsixOperationResult<MsixManifest> {

			auto impl = std::unique_ptr<Impl, ImplDeleter>{ new Impl{} };
			auto result = impl->Initialize(std::move(buffer), bufferSpan);

			if (!result)
				return result.error();
			return MsixManifest{ std::move(impl) };
		}

		auto Identity() const noexcept -> PackageIdentity const& {

			return identity;
		}

		auto Application() const noexcept -> MsixApplication const& {

			return application;
		}

		auto CustomInstallExtension() const noexcept -> MsixCustomInstallExtension const& {

			return customInstallExtension;
		}

		auto DetachFile() noexcept -> Citrine::File {

			return std::move(file);
		}

	private:

		Impl() noexcept = default;

		Impl(Citrine::File&& file) noexcept

			: file(std::move(file))
		{}

		auto Initialize() -> MsixOperationResult<> {

			if (!file)
				return MsixError::FileNotOpen;

			auto buffer = std::string{};
			if (!file.SeekToBegin() || !file.ReadToEnd(buffer))
				return MsixError::ReadingFailed;

			auto bufferSpan = MakeBufferSpan(buffer);
			return Initialize(MakeBufferHandle(std::move(buffer)), bufferSpan);
		}

		auto Initialize(BufferHandle&& buffer, std::span<std::uint8_t> bufferSpan) -> MsixOperationResult<> {

			rawXmlBuffer = std::move(buffer);
			rawXml = bufferSpan;
			return Parse();
		}

		auto Parse() -> MsixOperationResult<> {

			if (!xmlDocument.load_buffer_inplace(rawXml.data(), rawXml.size()))
				return MsixError::ParsingFailed;

			if (auto result = ParseIdentityElement(); !result)
				return result.error();

			if (auto result = ParseApplicationElement(); !result)
				return result.error();

			if (auto result = ParseCustomInstallExtensionElement(); !result)
				return result.error();

			return {};
		}

		auto ParseIdentityElement() -> MsixOperationResult<> {

			auto identityElement = xmlDocument.child("Package").child("Identity");
			if (!identityElement)
				return MsixError::ParsingFailed;

			auto nameAttribute = identityElement.attribute("Name");
			auto publisherAttribute = identityElement.attribute("Publisher");
			auto versionAttribute = identityElement.attribute("Version");
			auto architectureAttribute = identityElement.attribute("ProcessorArchitecture");

			if (!nameAttribute || !publisherAttribute || !versionAttribute || !architectureAttribute)
				return MsixError::ParsingFailed;

			auto name = std::string_view{ nameAttribute.as_string() };
			auto version = std::string_view{ versionAttribute.as_string() };
			auto architecture = std::string_view{ architectureAttribute.as_string() };
			auto publisherId = GetPublisherIdFromPublisher(publisherAttribute.as_string());

			auto fullName = std::string{};
			auto fullNameSize = name.size() + 1 + version.size() + 1 + architecture.size() + 1 + 0 + 1 + publisherId.size();
			fullName.resize_and_overwrite(fullNameSize, [&](char* data, std::size_t size) {

				auto out = data;

				out = std::ranges::copy(name, out).out;
				*out++ = '_';
				out = std::ranges::copy(version, out).out;
				*out++ = '_';
				out = std::ranges::copy(architecture, out).out;
				*out++ = '_';
				//
				*out++ = '_';
				out = std::ranges::copy(publisherId, out).out;

				return size;
			});

			identity = PackageIdentity{ std::move(fullName) };
			if (!identity.IsValid())
				return MsixError::ParsingFailed;

			return {};
		}

		auto ParseApplicationElement() -> MsixOperationResult<> {

			auto applicationsElement = xmlDocument.child("Package").child("Applications");
			if (!applicationsElement)
				return MsixError::ParsingFailed;

			auto applicationElement = applicationsElement.begin();
			if (applicationElement == applicationsElement.end())
				return MsixError::ParsingFailed;

			if (std::string_view{ applicationElement->name() } != "Application")
				return MsixError::ParsingFailed;

			auto idAttribute = applicationElement->attribute("Id");
			auto executableAttribute = applicationElement->attribute("Executable");
			auto entryPointAttribute = applicationElement->attribute("EntryPoint");

			if (!idAttribute)
				return MsixError::ParsingFailed;

			application.Id = idAttribute.as_string();
			application.Executable = executableAttribute.as_string();
			application.EntryPoint = entryPointAttribute.as_string();

			for (auto const& extensionElement : applicationElement->child("Extensions")) {

				if (std::string_view{ extensionElement.name() } != "uap:Extension")
					continue;

				if (std::string_view{ extensionElement.attribute("Category").as_string() } != "windows.fileTypeAssociation")
					continue;

				auto entryPoint = extensionElement.attribute("EntryPoint").as_string();

				auto fileTypeAssociationElement = extensionElement.begin();
				if (fileTypeAssociationElement == extensionElement.end())
					return MsixError::ParsingFailed;

				if (std::string_view{ fileTypeAssociationElement->name() } != "uap:FileTypeAssociation")
					return MsixError::ParsingFailed;

				for (auto const& fileTypeElement : fileTypeAssociationElement->child("uap:SupportedFileTypes")) {

					auto fileTypeText = fileTypeElement.text();
					if (!fileTypeText)
						return MsixError::ParsingFailed;

					application.AssociatedFileTypes.emplace_back(fileTypeText.as_string(), entryPoint);
				}

				if (++fileTypeAssociationElement != extensionElement.end())
					return MsixError::ParsingFailed;
			}

			if (++applicationElement != applicationsElement.end())
				return MsixError::UnsupportedFormat;

			return {};
		}

		auto ParseCustomInstallExtensionElement() -> MsixOperationResult<> {

			constexpr auto parseActionsElement = []<typename T>(pugi::xml_node const& actionsElement, std::vector<T>& actions) static -> bool {

				constexpr auto& expectedElementName = [] static -> auto& {

					if constexpr (std::same_as<T, MsixInstallAction>)
						return "desktop6:InstallAction";
					else if constexpr (std::same_as<T, MsixRepairAction>)
						return "desktop6:RepairAction";
					else if constexpr (std::same_as<T, MsixUninstallAction>)
						return "desktop6:UninstallAction";
				}();

				for (auto const& actionElement : actionsElement) {

					if (std::string_view{ actionElement.name() } != expectedElementName)
						return false;

					auto fileAttribute = actionElement.attribute("File");
					auto nameAttribute = actionElement.attribute("Name");
					auto argumentsAttribute = actionElement.attribute("Arguments");

					if (!fileAttribute || !nameAttribute)
						return false;

					actions.push_back({ fileAttribute.as_string(), nameAttribute.as_string(), argumentsAttribute.as_string() });
				}

				return true;
			};

			for (auto const& extensionElement : xmlDocument.child("Package").child("Extensions")) {

				if (std::string_view{ extensionElement.name() } != "desktop6:Extension")
					continue;

				if (std::string_view{ extensionElement.attribute("Category").as_string() } != "windows.customInstall")
					continue;

				auto customInstallElement = extensionElement.begin();
				if (customInstallElement == extensionElement.end())
					return MsixError::ParsingFailed;

				if (std::string_view{ customInstallElement->name() } != "desktop6:CustomInstall")
					return MsixError::ParsingFailed;

				auto folderAttribute = customInstallElement->attribute("Folder");
				auto runAsUserAttribute = customInstallElement->attribute("RunAsUser");

				if (!folderAttribute)
					return MsixError::ParsingFailed;

				customInstallExtension.FileDirectory = folderAttribute.as_string();
				customInstallExtension.RunAsUser = runAsUserAttribute.as_bool();

				for (auto const& actionsElement : *customInstallElement) {

					auto elementName = std::string_view{ actionsElement.name() };
					if (elementName == "desktop6:InstallActions") {

						if (!parseActionsElement(actionsElement, customInstallExtension.InstallActions))
							return MsixError::ParsingFailed;
					}
					else if (elementName == "desktop6:RepairActions") {

						if (!parseActionsElement(actionsElement, customInstallExtension.RepairActions))
							return MsixError::ParsingFailed;
					}
					else if (elementName == "desktop6:UninstallActions") {

						if (!parseActionsElement(actionsElement, customInstallExtension.UninstallActions))
							return MsixError::ParsingFailed;
					}
				}

				if (++customInstallElement != extensionElement.end())
					return MsixError::ParsingFailed;
			}

			return {};
		}

		Citrine::File file;
		BufferHandle rawXmlBuffer{ nullptr, nullptr };
		std::span<std::uint8_t> rawXml;
		pugi::xml_document xmlDocument;

		PackageIdentity identity;
		MsixApplication application;
		MsixCustomInstallExtension customInstallExtension;
	};

	auto MsixManifest::ImplDeleter::operator()(Impl* impl) const -> void {

		delete impl;
	}

	MsixManifest::MsixManifest(std::unique_ptr<Impl, ImplDeleter>&& impl) noexcept

		: impl(std::move(impl))
	{}

	auto MsixManifest::operator=(std::nullptr_t) noexcept -> MsixManifest& {

		impl.reset();
		return *this;
	}

	auto MsixManifest::OpenFromFile(Citrine::File&& file) -> MsixOperationResult<MsixManifest> {

		return Impl::OpenFromFile(std::move(file));
	}

	auto MsixManifest::OpenFromBuffer(BufferHandle&& buffer, std::span<std::uint8_t> bufferSpan) -> MsixOperationResult<MsixManifest> {

		return Impl::OpenFromBuffer(std::move(buffer), bufferSpan);
	}

	auto MsixManifest::Identity() const noexcept -> PackageIdentity const& {

		return impl->Identity();
	}

	auto MsixManifest::Application() const noexcept -> MsixApplication const& {

		return impl->Application();
	}

	auto MsixManifest::CustomInstallExtension() const noexcept -> MsixCustomInstallExtension const& {

		return impl->CustomInstallExtension();
	}

	MsixManifest::operator bool() const noexcept {

		return static_cast<bool>(impl);
	}

	auto MsixManifest::Release() noexcept -> void {

		impl.reset();
	}

	auto MsixManifest::File() && noexcept -> Citrine::File {

		return impl->DetachFile();
	}

	auto MsixManifest::swap(MsixManifest& other) -> void {

		impl.swap(other.impl);
	}
}