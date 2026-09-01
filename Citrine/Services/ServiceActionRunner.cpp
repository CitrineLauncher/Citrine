#include "pch.h"
#include "ServiceActionRunner.h"

#include "ServiceActionHandler.h"

namespace Citrine {

	auto ServiceActionRunner::ExecuteAction(std::string_view executionContextStr) -> bool {

		return ServiceActionHandler::ExecuteAction(executionContextStr);
	}
}