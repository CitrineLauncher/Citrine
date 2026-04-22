#pragma once

#include "Core/Coroutine/Task.h"
#include "Core/Net/Url.h"

#include <concepts>
#include <string>
#include <expected>

namespace Citrine::Windows {

	enum struct FE3Error {

		Unknown,
		NetworkError,
		ApiError,
		ResponseError,
		ContentNotFound
	};

	template<typename T = void>
	struct FE3Result : std::expected<T, FE3Error> {

		using FE3Result::expected::expected;

		FE3Result(FE3Error error) noexcept

			: FE3Result::expected(std::unexpect, error)
		{}
	};

	template<typename T = void>
	using AsyncFE3Result = Task<FE3Result<T>>;

	class FE3Handler {
	public:

		static auto GetFileUrlAsync(std::string updateId, std::uint64_t revisionNumber) -> AsyncFE3Result<Url>;
	};
}
