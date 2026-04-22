#pragma once

#include <coroutine>
#include <exception>

namespace Citrine {

	struct FireAndForget {

		struct promise_type {

			auto get_return_object() const noexcept -> FireAndForget {

				return {};
			}

			auto return_void() const noexcept -> void {
			
			}

			auto initial_suspend() const noexcept -> std::suspend_never {

				return {};
			}

			auto final_suspend() const noexcept -> std::suspend_never {

				return {};
			}

			auto unhandled_exception() const noexcept -> void {

				std::terminate();
			}
		};
	};
}