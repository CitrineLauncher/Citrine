#include "pch.h"
#include "PackageModel.h"

namespace Citrine::Xbox {

	auto PackageVersionIdentifier::Parse(std::string_view str) noexcept -> std::optional<PackageVersionIdentifier> {

		auto value = std::optional<PackageVersionIdentifier>{ std::in_place };
		if (!Parser::Parse(str, *value)) value.reset();
		return value;
	}

	auto PackageVersionIdentifier::Parse(std::string_view str, PackageVersionIdentifier& value) noexcept -> bool {

		return Parser::Parse(str, value);
	}

	auto PackageVersionIdentifier::Format() const -> std::string {

		auto str = std::string{};
		Formatter::FormatTo(str, *this);
		return str;
	}

	auto PackageVersionIdentifier::FormatTo(std::string& str) const -> void {

		Formatter::FormatTo(str, *this);
	}

	PackageVersionIdentifier::operator std::string() const {

		return Format();
	}

	auto PackageVersionIdentifier::Parser::Parse(std::string_view str, PackageVersionIdentifier& value) noexcept -> bool {

		auto delimPos = str.find_last_of('.');
		if (delimPos == str.npos)
			return false;

		if (!Windows::PackageVersion::Parse({ str.begin(), str.begin() + delimPos }, value.Version))
			return false;

		if (!Guid::Parse({ str.begin() + delimPos + 1, str.end() }, value.Id))
			return false;

		return true;
	}

	auto PackageVersionIdentifier::Formatter::FormatTo(char* out, PackageVersionIdentifier const& value) noexcept -> char* {

		auto& [version, id] = value;

		out = VersionNumberFormatter::FormatTo(out, version);
		*out++ = '.';
		out = GuidFormatter::FormatTo(out, id);

		return out;
	}

	auto PackageVersionIdentifier::Formatter::FormatTo(std::string& output, PackageVersionIdentifier const& value) -> void {

		auto buffer = TrivialArray<char, MaxFormattedSize()>{};
		output.assign(buffer.data(), FormatTo(buffer.data(), value));
	}
}