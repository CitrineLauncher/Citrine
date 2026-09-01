#pragma once

namespace Citrine {

	class ServiceActionRunner {
	public:

		static auto ExecuteAction(std::string_view executionContextStr) -> bool;
	};
}
