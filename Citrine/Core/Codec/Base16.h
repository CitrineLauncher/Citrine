#pragma once

#include "Base16Encoder.h"
#include "Base16Decoder.h"

#include <span>

namespace Citrine {

	class Base16 {
	public:

		static constexpr auto UpperAlphabet = std::array<std::uint8_t, 16>{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
		static constexpr auto LowerAlphabet = std::array<std::uint8_t, 16>{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return Base16Encoder::EncodedSize(inSize);
		}

		static constexpr auto EncodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base16Encoder::EncodedSize(input.size());
		}

		static constexpr auto EncodedSize(std::string_view input) noexcept -> std::size_t {

			return Base16Encoder::EncodedSize(input.size());
		}

		template<typename T>
		static constexpr auto EncodeUpper(std::array<T, 1> in, auto* out) noexcept -> decltype(out) {

			return Base16Encoder::Encode<UpperAlphabet>(in, out);
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Encoder::Encode<UpperAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Encoder::Encode<UpperAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename T>
		static constexpr auto EncodeLower(std::array<T, 1> in, auto* out) noexcept -> decltype(out) {

			return Base16Encoder::Encode<LowerAlphabet>(in, out);
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Encoder::Encode<LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Encoder::Encode<LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto DecodedSize(std::size_t inSize) noexcept -> std::size_t {

			return Base16Decoder::DecodedSize(inSize);
		}

		static constexpr auto DecodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Base16Decoder::DecodedSize(input.size());
		}

		static constexpr auto DecodedSize(std::string_view input) noexcept -> std::size_t {

			return Base16Decoder::DecodedSize(input.size());
		}

		template<typename T>
		static constexpr auto Decode(std::array<T, 2> in, auto* out) noexcept -> decltype(out) {

			return Base16Decoder::Decode<UpperAlphabet, LowerAlphabet>(in, out);
		}

		template<typename Output>
		static constexpr auto Decode(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Decoder::Decode<UpperAlphabet, LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto Decode(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Base16Decoder::Decode<UpperAlphabet, LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename T>
		static constexpr auto Validate(std::array<T, 2> in) noexcept -> bool {

			return Base16Decoder::Validate<UpperAlphabet, LowerAlphabet>(in);
		}

		static constexpr auto Validate(std::span<std::uint8_t const> input) noexcept -> bool {

			return Base16Decoder::Validate<UpperAlphabet, LowerAlphabet>(input.data(), input.size());
		}

		static constexpr auto Validate(std::string_view input) noexcept -> bool {

			return Base16Decoder::Validate<UpperAlphabet, LowerAlphabet>(input.data(), input.size());
		}
	};
}
