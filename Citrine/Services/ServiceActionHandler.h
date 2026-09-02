#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Coroutine/Concepts.h"
#include "Core/Coroutine/Traits.h"

#include <string>
#include <type_traits>
#include <tuple>
#include <utility>

#include <glaze/json.hpp>

namespace Citrine {

	class ServiceActionHandler {
	private:

		friend class ServiceImplBase;
		friend class ServiceActionRunner;

		template<typename T>
		static auto RegisterAction(std::string_view serviceName, std::string_view actionName) -> bool {

			constexpr auto invocable = [](std::string const* argsJson, std::string* resultJson) static -> LazyTask<bool> {

				using Args = ActionTraits<T>::Args;
				auto args = std::conditional_t<std::same_as<Args, void>,
					std::monostate,
					Args
				>{};

				using Result = ActionTraits<T>::Result;
				auto result = std::conditional_t<std::same_as<Result, void>,
					std::monostate,
					Result
				>{};

				if (!Unmarshal(args, *argsJson))
					co_return false;

				auto invoke = [&args] -> decltype(auto) {

					if constexpr (!std::same_as<Args, void>)
						return T::Execute(std::move(args));
					else
						return T::Execute();
				};

				if constexpr (Awaitable<decltype(invoke())>) {

					if constexpr (!std::same_as<Result, void>)
						result = co_await invoke();
					else
						co_await invoke();
				}
				else {

					if constexpr (!std::same_as<Result, void>)
						result = invoke();
					else
						invoke();
				}

				if (!Marshal(result, *resultJson))
					co_return false;

				co_return true;
			};

			return RegisterActionInternal(serviceName, actionName, invocable);
		}

		template<typename T, typename... Params>
		static auto RunActionInAdminContext(std::string_view serviceName, std::string_view actionName, Params&... params) -> LazyTask<bool> {

			using Args = ActionTraits<T>::Args;
			decltype(auto) args = [params = std::tie(params...)] -> decltype(auto) {

				if constexpr (!std::same_as<Args, void>)
					return std::as_const(std::get<0>(params));
				else
					return std::monostate{};
			}();

			using Result = ActionTraits<T>::Result;
			decltype(auto) result = [params = std::tie(params...)] -> decltype(auto) {

				if constexpr (!std::same_as<Result, void>)
					return std::get<std::same_as<Args, void> ? 0 : 1>(params);
				else
					return std::monostate{};
			}();

			auto argsJson = std::string{};
			auto resultJson = std::string{};

			if (!Marshal(args, argsJson))
				co_return false;

			if (!co_await RunActionInAdminContextInternal(serviceName, actionName, &argsJson, &resultJson))
				co_return false;

			if (!Unmarshal(result, resultJson))
				co_return false;

			co_return true;
		}

		static auto ExecuteAction(std::string_view executionContextStr) -> bool {

			return ExecuteActionInternal(executionContextStr);
		}

		using ActionFunc = auto(*)(std::string const*, std::string*) -> LazyTask<bool>;

		template<typename...>
		struct ActionTraits;

		template<typename R>
		struct ActionTraits<auto(*)() -> R> {

			using Args = void;
			using Result = R;
		};

		template<Awaitable R>
		struct ActionTraits<auto(*)() -> R> {

			using Args = void;
			using Result = AwaitableTraits<R>::AwaiterResult;
		};

		template<typename A, typename R>
		struct ActionTraits<auto(*)(A) -> R> {

			using Args = A;
			using Result = R;
		};

		template<typename A, Awaitable R>
		struct ActionTraits<auto(*)(A) -> R> {

			using Args = A;
			using Result = AwaitableTraits<R>::AwaiterResult;
		};

		template<typename T>
		struct ActionTraits<T> : ActionTraits<decltype(&T::Execute)> {};

		template<typename T>
		static auto Marshal(T const& obj, std::string& buffer) -> bool {

			constexpr auto opts = glz::opts{};
			return !glz::write<opts>(obj, buffer);
		}

		template<typename T>
		static auto Unmarshal(T& obj, std::string_view buffer) -> bool {

			constexpr auto opts = glz::opts{ .null_terminated = false, .error_on_missing_keys = true };
			return !glz::read<opts>(obj, buffer);
		}

		struct RegisteredAction {

			std::string Id;
			ActionFunc Func;
		};

		class RegisteredActions;

		static auto GetRegisteredActions() -> RegisteredActions&;

		static auto RegisterActionInternal(std::string_view serviceName, std::string_view actionName, ActionFunc actionFunc) -> bool;
		static auto RunActionInAdminContextInternal(std::string_view serviceName, std::string_view actionName, std::string const* argsJson, std::string* resultJson) -> LazyTask<bool>;
		static auto ExecuteActionInternal(std::string_view executionContextStr) -> bool;
	};
}