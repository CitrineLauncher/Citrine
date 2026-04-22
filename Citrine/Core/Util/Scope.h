#pragma once

#include <type_traits>

namespace Citrine {

	template<typename Func>
	class ScopeExit {
	public:

		template<typename F>
			requires std::is_nothrow_constructible_v<Func, F>
		explicit ScopeExit(F&& func) noexcept : func(std::forward<F>(func)) {

		}

		template<typename F>
			requires (!std::is_nothrow_constructible_v<Func, F>) && std::is_lvalue_reference_v<F>
		explicit ScopeExit(F&& func) try : func(func) {

		}
		catch (...) {

			func();
			throw;
		}

		ScopeExit(ScopeExit const&) = delete;
		auto operator=(ScopeExit const&) = delete;

		auto Release() noexcept -> void {

			execute = false;
		}

		~ScopeExit() noexcept {

			if (execute)
			    std::move(func)();
		}

	private:

		Func func;
		bool execute{ true };
	};

	template<typename F>
	ScopeExit(F) -> ScopeExit<F>;
}