#pragma once

namespace Citrine::Minecraft::Bedrock {

	enum struct PackageOperationStatus : std::uint8_t {

		Idle,
		Running,
		Completed,
		PauseRequested,
		Paused,
		CancellationRequested,
		Cancelled,
		Failed,
	};
}