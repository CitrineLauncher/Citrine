#pragma once

#include <string>

namespace Citrine::Windows {

	struct MsixFileTypeAssociation {

		std::string Extension;
		std::string AppEntryPoint;
	};
}