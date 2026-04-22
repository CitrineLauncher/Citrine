#pragma once

#include "MsixFileTypeAssociation.h"

#include <string>
#include <filesystem>

namespace Citrine::Windows {

	struct MsixApplication {

		std::string Id;
		std::filesystem::path Executable;
		std::string EntryPoint;
		std::vector<MsixFileTypeAssociation> AssociatedFileTypes;
	};
}