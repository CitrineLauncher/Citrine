#pragma once

#include "Base32Encoder.h"
#include "Base32Decoder.h"

#include <span>

namespace Citrine {

	class Base32 {
	public:

		static constexpr auto UpperAlphabet = std::array<std::uint8_t, 32>{

			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', '2', '3', '4', '5', '6', '7',
		};
		static constexpr auto LowerAlphabet = std::array<std::uint8_t, 32>{

			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
			'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
			'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
			'y', 'z', '2', '3', '4', '5', '6', '7'
		};
		static constexpr auto PaddingChar = std::uint8_t{ '=' };

		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return Base32Encoder::EncodedSize<PaddingChar>(inSize);
		}

		static constexpr auto EncodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base32Encoder::EncodedSize<PaddingChar>(input.size());
		}

		static constexpr auto EncodedSize(std::string_view input) noexcept -> std::size_t {

			return Base32Encoder::EncodedSize<PaddingChar>(input.size());
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Encoder::Encode<UpperAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Encoder::Encode<UpperAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Encoder::Encode<LowerAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Encoder::Encode<LowerAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto DecodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base32Decoder::DecodedSize<PaddingChar>(input.data(), input.size());
		}

		static constexpr auto DecodedSize(std::string_view input) noexcept -> std::size_t {

			return Base32Decoder::DecodedSize<PaddingChar>(input.data(), input.size());
		}

		template<typename Output>
		static constexpr auto DecodeUpper(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Decoder::Decode<UpperAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto DecodeUpper(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Decoder::Decode<UpperAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto DecodeLower(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Decoder::Decode<LowerAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto DecodeLower(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base32Decoder::Decode<LowerAlphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto ValidateUpper(std::span<std::uint8_t const> input) noexcept -> bool {

			return Base32Decoder::Validate<UpperAlphabet, PaddingChar>(input.data(), input.size());
		}

		static constexpr auto ValidateUpper(std::string_view input) noexcept -> bool {

			return Base32Decoder::Validate<UpperAlphabet, PaddingChar>(input.data(), input.size());
		}

		static constexpr auto ValidateLower(std::span<std::uint8_t const> input) noexcept -> bool {

			return Base32Decoder::Validate<LowerAlphabet, PaddingChar>(input.data(), input.size());
		}

		static constexpr auto ValidateLower(std::string_view input) noexcept -> bool {

			return Base32Decoder::Validate<LowerAlphabet, PaddingChar>(input.data(), input.size());
		}
	};
}