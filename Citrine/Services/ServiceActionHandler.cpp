#include "pch.h"
#include "ServiceActionHandler.h"

#include "Core/Util/Ascii.h"
#include "Core/Util/Guid.h"
#include "Core/Util/Scope.h"
#include "Core/Util/Append.h"
#include "Core/IO/Buffer.h"
#include "Core/Coroutine/FireAndForget.h"
#include "Windows/Shell.h"

#include <flat_set>
#include <shared_mutex>
#include <optional>
#include <array>
#include <algorithm>
#include <expected>
#include <atomic>

#include <glaze/json.hpp>

#include <wil/stl.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

using namespace Citrine;

using namespace std::string_view_literals;

namespace {

	auto GetActionId(std::string_view serviceName, std::string_view actionName) -> std::string {

		auto id = std::string{};
		auto idSize = serviceName.size() + 1 + actionName.size();

		id.resize_and_overwrite(idSize, [&](char* data, std::size_t size) {

			auto out = data;
			out = std::ranges::copy(serviceName, out).out;
			*out++ = '/';
			out = std::ranges::copy(actionName, out).out;
			return size;
		});
		return id;
	}

	struct ExecutionContext {

		static auto Parse(std::string_view str, ExecutionContext& value) -> bool {

			auto delimPos = str.find_last_of('_');
			if (delimPos == str.npos)
				return false;

			value.ActionId = { str.begin(), str.begin() + delimPos };

			if (!Guid::Parse({ str.begin() + delimPos + 1, str.end() }, value.PipeId))
				return false;

			return true;
		}

		auto Serialize(std::string& output) -> void {

			output.clear();
			Serialize(AppendTo(output));
		}

		auto Serialize(AppendTo<std::string> output) -> void {

			auto oldSize = output->size();
			auto newSize = oldSize + ActionId.size() + 1 + GuidFormatter::FormattedSize();

			output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

				auto out = data + oldSize;
				out = std::ranges::copy(ActionId, out).out;
				*out++ = '_';
				out = GuidFormatter::FormatTo(out, PipeId);
				return size;
			});
		}

		std::string ActionId;
		Guid PipeId;
	};

	auto GetPipeName(Guid pipeId) -> std::wstring {

		constexpr auto baseName = LR"(\\.\pipe\CitrineLauncher_)"sv;

		auto name = std::wstring{};
		auto nameSize = baseName.size() + GuidFormatter::FormattedSize();

		name.resize_and_overwrite(nameSize, [&](wchar_t* data, std::size_t size) {

			auto out = data;
			out = std::ranges::copy(baseName, out).out;
			out = GuidFormatter::FormatTo(out, pipeId);
			return size;
		});
		return name;
	}

	constexpr auto PipeBufferSize = 16 * 1024;

	enum struct MessageType {

		Unknown,
		ExecuteAction,
		ActionCompleted
	};

	struct Message {

		MessageType Type{};
		std::optional<glz::raw_json_view> Payload;
	};

	using PipeOperationResult = std::expected<::DWORD, ::DWORD>;

	template<typename Func>
	auto InvokePipeOperation(::HANDLE pipe, Func func) -> PipeOperationResult {

		auto overlapped = ::OVERLAPPED{};
		if (!func(overlapped)) {

			auto error = ::GetLastError();
			if (error != ERROR_PIPE_CONNECTED && error != ERROR_IO_PENDING)
				return std::unexpected{ error };
		}

		auto bytesTransferred = ::DWORD{};
		if (!::GetOverlappedResult(pipe, &overlapped, &bytesTransferred, true))
			return std::unexpected{ ::GetLastError() };

		return bytesTransferred;
	}
	
	template<typename Func>
	auto InvokePipeOperation(::HANDLE pipe, Func func, ::HANDLE cancelledEvent, ::HANDLE childProcess) -> PipeOperationResult {

		auto completedEvent = wil::unique_event{};
		completedEvent.create();

		auto overlapped = ::OVERLAPPED{ .hEvent = completedEvent.get() };
		if (!func(overlapped)) {

			auto error = ::GetLastError();
			if (error != ERROR_PIPE_CONNECTED && error != ERROR_IO_PENDING)
				return std::unexpected{ error };
		}

		auto handles = std::array{ cancelledEvent, completedEvent.get(), childProcess };
		auto result = ::WaitForMultipleObjects(handles.size(), handles.data(), false, INFINITE);

		if (result - WAIT_OBJECT_0 == 1) {

			auto bytesTransferred = ::DWORD{};
			if (!::GetOverlappedResult(pipe, &overlapped, &bytesTransferred, false))
				return std::unexpected{ ::GetLastError() };

			return bytesTransferred;
		}
		else {

			auto error = ::GetLastError();
			::CancelIoEx(pipe, &overlapped);

			auto bytesTransferred = ::DWORD{};
			::GetOverlappedResult(pipe, &overlapped, &bytesTransferred, true);

			if (result - WAIT_OBJECT_0 == 0)
				throw winrt::hresult_canceled{};

			if (result - WAIT_OBJECT_0 == 2)
				return std::unexpected{ ERROR_BROKEN_PIPE };

			return std::unexpected{ error };
		}
	}
}

namespace glz {

	template<>
	struct meta<MessageType> {

		using enum MessageType;

		static constexpr auto value = enumerate(
			"ExecuteAction", ExecuteAction,
			"ActionCompleted", ActionCompleted
		);
	};

	template<>
	struct meta<Message> {

		using T = Message;

		static constexpr auto value = object(
			"Type", &T::Type,
			"Payload", &T::Payload
		);
	};
}

namespace Citrine {

	class ServiceActionHandler::RegisteredActions {
	public:

		RegisteredActions() = default;

		RegisteredActions(RegisteredActions const&) = delete;
		auto operator=(RegisteredActions const&) = delete;

		auto Lock() noexcept -> auto {

			return CollectionLock<std::unique_lock, CollectionT>{ this };
		}

		auto LockShared() noexcept -> auto {

			return CollectionLock<std::shared_lock, CollectionT const>{ this };
		}

	private:

		struct KeyCompare {

			using is_transparent = void;

			static auto operator()(auto const& left, auto const& right) noexcept -> bool {

				constexpr auto toLower = [](char ch) static { return Ascii::ToLower(ch); };

				return std::ranges::lexicographical_compare(GetId(left), GetId(right), {}, toLower, toLower);
			}

			static auto GetId(RegisteredAction const& action) noexcept -> std::string_view {

				return action.Id;
			}

			static auto GetId(std::string_view id) noexcept -> std::string_view {

				return id;
			}
		};

		using CollectionT = std::flat_set<RegisteredAction, KeyCompare>;

		template<template <typename> typename Lock, typename Collection>
		struct CollectionLock {

			CollectionLock(RegisteredActions* parent)

				: lock(parent->mutex)
				, collection(&parent->collection)
			{}

			CollectionLock(CollectionLock const&) = delete;
			auto operator=(CollectionLock const&) = delete;

			auto operator->() const noexcept -> Collection* {

				return collection;
			}

		private:

			Lock<std::shared_mutex> lock;
			Collection* collection;
		};

		std::shared_mutex mutex;
		CollectionT collection;
	};

	auto ServiceActionHandler::GetRegisteredActions() -> RegisteredActions& {

		static auto registeredActions = RegisteredActions{};
		return registeredActions;
	}

	auto ServiceActionHandler::RegisterActionInternal(std::string_view serviceName, std::string_view actionName, ActionFunc actionFunc) -> bool {

		auto actionId = GetActionId(serviceName, actionName);

		auto registeredActions = GetRegisteredActions().Lock();
		auto [it, inserted] = registeredActions->emplace(std::move(actionId), actionFunc);
		return inserted;
	}

	auto ServiceActionHandler::RunActionInAdminContextInternal(std::string_view serviceName, std::string_view actionName, std::string const* argsJson, std::string* resultJson) -> LazyTask<bool> {

		using enum MessageType;

		auto executionContext = ExecutionContext{ GetActionId(serviceName, actionName), Guid::Create() };
		auto& [actionId, pipeId] = executionContext;

		co_await winrt::resume_background();

		auto cancelledEvent = wil::shared_event{};
		cancelledEvent.create(wil::EventOptions::ManualReset);

		auto cancellationToken = co_await GetCancellationToken();
		cancellationToken.Callback([cancelledEvent] {

			cancelledEvent.SetEvent();
		});

		auto pipeName = GetPipeName(pipeId);
		auto pipe = wil::unique_hfile{ ::CreateNamedPipeW(
			pipeName.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS | PIPE_WAIT,
			1,
			PipeBufferSize,
			PipeBufferSize,
			NMPWAIT_USE_DEFAULT_WAIT,
			nullptr
		) };

		if (!pipe)
			co_return false;

		auto childProcessArgs = std::string{};
		childProcessArgs.append("RunServiceAction");
		childProcessArgs.push_back(' ');
		executionContext.Serialize(AppendTo(childProcessArgs));

		auto childProcess = co_await Windows::Shell::ExecuteAsync(wil::GetModuleFileNameW<std::wstring>(), childProcessArgs, true);
		if (!childProcess)
			co_return false;

		auto jobObject = [&] -> wil::unique_handle {

			auto jobObject = wil::unique_handle{ ::CreateJobObjectW(nullptr, nullptr) };
			if (!jobObject)
				return nullptr;

			auto jobInfo = JOBOBJECT_EXTENDED_LIMIT_INFORMATION{
			
				.BasicLimitInformation = {

					.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
				}
			};

			if (!::SetInformationJobObject(jobObject.get(), JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo)))
				return nullptr;

			if (!::AssignProcessToJobObject(jobObject.get(), childProcess->get()))
				return nullptr;

			return jobObject;
		}();

		if (!jobObject) {

			::TerminateProcess(childProcess->get(), 0);
			co_return false;
		}

		{
			auto result = InvokePipeOperation(
				pipe.get(),
				[&](::OVERLAPPED& overlapped) {

					return ::ConnectNamedPipe(pipe.get(), &overlapped);
				},
				cancelledEvent.get(),
				childProcess->get()
			);

			if (!result)
				co_return false;
		}

		auto disconnectPipe = ScopeExit{ [&pipe] { ::DisconnectNamedPipe(pipe.get()); } };

		{
			auto message = Message{

				.Type = ExecuteAction,
				.Payload = *argsJson
			};

			auto messageJson = std::string{};
			if (!Marshal(message, messageJson))
				co_return false;

			auto result = InvokePipeOperation(
				pipe.get(),
				[&](::OVERLAPPED& overlapped) {

					return ::WriteFile(pipe.get(), messageJson.data(), messageJson.size(), nullptr, &overlapped);
				},
				cancelledEvent.get(),
				childProcess->get()
			);

			if (!result)
				co_return false;
		}

		{
			auto messageBuffer = Buffer{ PipeBufferSize };
			auto result = InvokePipeOperation(
				pipe.get(),
				[&](::OVERLAPPED& overlapped) {

					return ::ReadFile(pipe.get(), messageBuffer.data(), messageBuffer.capacity(), nullptr, &overlapped);
				},
				cancelledEvent.get(),
				childProcess->get()
			);

			if (!result)
				co_return false;

			auto message = Message{};
			if (!Unmarshal(message, { reinterpret_cast<char*>(messageBuffer.data()), *result }))
				co_return false;

			auto messageType = message.Type;
			if (messageType != ActionCompleted)
				co_return false;

			auto& messagePayload = message.Payload;
			if (!messagePayload)
				co_return false;

			*resultJson = std::string{ messagePayload->str };
		}

		co_return true;
	}

	auto ServiceActionHandler::ExecuteActionInternal(std::string_view executionContextStr) -> bool {

		using enum MessageType;

		auto executionContext = ExecutionContext{};
		auto& [actionId, pipeId] = executionContext;

		if (!ExecutionContext::Parse(executionContextStr, executionContext))
			return false;

		auto actionFunc = [&] -> ActionFunc {

			auto registeredActions = GetRegisteredActions().LockShared();
			auto it = registeredActions->find(actionId);
			return (it != registeredActions->end()) ? it->Func : nullptr;
		}();

		if (!actionFunc)
			return false;

		auto pipeName = GetPipeName(pipeId);
		if (!::WaitNamedPipeW(pipeName.c_str(), 15000))
			return false;

		auto pipe = wil::unique_hfile{ ::CreateFileW(
			pipeName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			nullptr
		) };

		if (!pipe)
			return false;

		auto pipeMode = ::DWORD{ PIPE_READMODE_MESSAGE | PIPE_WAIT };
		if (!SetNamedPipeHandleState(pipe.get(), &pipeMode, nullptr, nullptr))
			return false;

		auto argsJson = std::string{};
		auto resultJson = std::string{};

		{
			auto messageBuffer = Buffer{ PipeBufferSize };
			auto result = InvokePipeOperation(
				pipe.get(),
				[&](::OVERLAPPED& overlapped) {

					return ::ReadFile(pipe.get(), messageBuffer.data(), messageBuffer.capacity(), nullptr, &overlapped);
				}
			);

			if (!result)
				return false;

			auto message = Message{};
			if (!Unmarshal(message, { reinterpret_cast<char*>(messageBuffer.data()), *result }))
				return false;

			auto messageType = message.Type;
			if (messageType != ExecuteAction)
				return false;

			auto& messagePayload = message.Payload;
			if (!messagePayload)
				return false;

			argsJson = std::string{ messagePayload->str };
		}

		auto completed = std::atomic_flag{};
		auto result = false;

		[&](this auto self) -> FireAndForget {

			auto onCompleted = ScopeExit{ [&completed] { 
				
				completed.test_and_set(std::memory_order::release);
				completed.notify_one();
			} };
			result = co_await actionFunc(&argsJson, &resultJson);
		}();

		completed.wait(false, std::memory_order::acquire);
		if (!result)
			return false;

		{
			auto message = Message{

				.Type = ActionCompleted,
				.Payload = resultJson
			};

			auto messageJson = std::string{};
			if (!Marshal(message, messageJson))
				return false;

			auto result = InvokePipeOperation(
				pipe.get(),
				[&](::OVERLAPPED& overlapped) {

					return ::WriteFile(pipe.get(), messageJson.data(), messageJson.size(), nullptr, &overlapped);
				}
			);

			if (!result)
				return false;
		}

		return true;
	}
}