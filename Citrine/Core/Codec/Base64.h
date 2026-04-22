#pragma once

#include "Base64Encoder.h"
#include "Base64Decoder.h"

#include <span>

namespace Citrine {

	class Base64 {
	public:

		static constexpr auto Alphabet = std::array<std::uint8_t, 64>{

			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8', '9', '+', '/'
		};
		static constexpr auto PaddingChar = std::uint8_t{ '=' };

		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return Base64Encoder::EncodedSize<PaddingChar>(inSize);
		}

		static constexpr auto EncodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base64Encoder::EncodedSize<PaddingChar>(input.size());
		}

		static constexpr auto EncodedSize(std::string_view input) noexcept -> std::size_t {

			return Base64Encoder::EncodedSize<PaddingChar>(input.size());
		}

		template<typename Output>
		static constexpr auto Encode(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base64Encoder::Encode<Alphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto Encode(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base64Encoder::Encode<Alphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto DecodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base64Decoder::DecodedSize<PaddingChar>(input.data(), input.size());
		}

		static constexpr auto DecodedSize(std::string_view input) noexcept -> std::size_t {

			return Base64Decoder::DecodedSize<PaddingChar>(input.data(), input.size());
		}

		template<typename Output>
		static constexpr auto Decode(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base64Decoder::Decode<Alphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto Decode(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base64Decoder::Decode<Alphabet, PaddingChar>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto Validate(std::span<std::uint8_t const> input) noexcept -> bool {

			return Base64Decoder::Validate<Alphabet, PaddingChar>(input.data(), input.size());
		}

		static constexpr auto Validate(std::string_view input) noexcept -> bool {

			return Base64Decoder::Validate<Alphabet, PaddingChar>(input.data(), input.size());
		}
	};
}