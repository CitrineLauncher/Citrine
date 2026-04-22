#include "pch.h"
#include "Logger.h"

#include "LogMessageQueue.h"

#include "Core/IO/File.h"
#include "Core/Util/FormatInteger.h"
#include "Core/Unicode/Utf.h"

using namespace Citrine;
using namespace std::literals;

namespace {

	auto WriteLogMessage(std::string& buffer, LogLevel level, DateTime dateTime, LogSourceLocation source, std::string_view message) -> void {

		{
			static constexpr auto fmt = DateTimeFormatString{ "[%Y-%M-%D %h:%m:%s.3f]" };

			auto const oldSize = buffer.size();
			auto const newSize = oldSize + fmt.FormattedSize();

			buffer.resize_and_overwrite(newSize, [oldSize, dateTime](char* data, std::size_t size) {

				DateTimeFormatter::FormatTo(data + oldSize, fmt, dateTime);
				return size;
			});
		}
		buffer.push_back(' ');
		{
			using enum LogLevel;

			auto levelStr = "unknown"sv;
			switch (level) {
			case Trace:		levelStr = "trace";		break;
			case Debug:		levelStr = "debug";		break;
			case Info:		levelStr = "info";		break;
			case Warn:		levelStr = "warn";		break;
			case Error:		levelStr = "error";		break;
			case Critical:	levelStr = "critical";	break;
			}
			buffer.append(levelStr);
		}
		buffer.push_back(' ');
		{
			constexpr auto projectDir = std::string_view{ PROJECT_DIR };

			auto fileName = std::string_view{ source.FileName };
			if (fileName.starts_with(projectDir))
				fileName.remove_prefix(projectDir.size());

			buffer.push_back('@');
			buffer.append(fileName);
			buffer.push_back(':');
			buffer.append(FormatInteger(source.Line));
		}
		buffer.append(" -> ");
		buffer.append(message);
		buffer.push_back('\n');
	}

	auto queue = LogMessageQueue{};
	auto worker = std::thread{};

	auto DispatchLogMessages(std::filesystem::path path) -> void {

		RotateFile(path, 16);
		auto file = File{ path, FileMode::CreateAlways, FileAccess::Write };

		auto buffer = std::string{};
		auto messages = LogMessageList{};
		auto getPayload = [narrow = std::string{}](LogMessagePayload const& payload) mutable -> std::string const& {

			if (std::holds_alternative<std::string>(payload)) {

				return std::get<std::string>(payload);
			}
			else {

				if (std::holds_alternative<std::wstring>(payload)) 
					ToUtf8(std::get<std::wstring>(payload), narrow);
				else
					narrow.clear();
				return narrow;
			}
		};

		while (true) {

			buffer.clear();
			auto flush = false;
			auto shutdown = false;

			queue.DequeueMessages(messages);
			for (auto const& message : messages) {

				using enum LogMessageType;
				switch (message.Type) {
				case Log:
					WriteLogMessage(buffer, message.Level, message.DateTime, message.Source, getPayload(message.Payload));
					break;
				case Flush:
					flush = true;
					break;
				case Shutdown:
					shutdown = true;
					break;
				}
			}

			if (!buffer.empty())
				file.Write(buffer);
			if (flush)
				file.Flush();
			if (shutdown)
				return;
		}
	}
}

namespace Citrine {

	auto Logger::Initialize(std::filesystem::path path) -> void {

		worker = std::thread{ DispatchLogMessages, std::move(path) };
	}

	auto Logger::Flush() -> void {

		PostLogMessage({ LogMessageType::Flush });
	}

	auto Logger::Shutdown() -> void {

		PostLogMessage({ LogMessageType::Shutdown });
		worker.join();
	}

	auto Logger::PostLogMessage(LogMessage&& message) -> void {

		queue.EnqueueMessage(std::move(message));
	}
}