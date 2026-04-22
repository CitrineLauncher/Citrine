#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base16Encoder {
	public:

		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return CalculateOutputSize(inSize);
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto in, auto* out) noexcept -> decltype(out)
			requires(std::size(in) == 1)
		{
			return EncodeOneByte<Alphabet>(in, out);
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			return EncodeBytes<Alphabet>(in, inSize, out);
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> void {

			output.clear();
			Encode<Alphabet>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> void {

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize(newSize);
			EncodeBytes<Alphabet>(in, inSize, output->data() + oldSize);
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::string& output) -> void {

			output.clear();
			Encode<Alphabet>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto Encode(auto const* in, std::size_t inSize, AppendTo<std::string> output) -> void {

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

				EncodeBytes<Alphabet>(in, inSize, data + oldSize);
				return size;
			});
		}

	private:

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto EncodeTable0 = [] static {

			auto encodeTable = std::array<std::uint16_t, 256>{};
			for (auto i = 0uz; i < encodeTable.size(); ++i) {

				auto value = static_cast<std::uint16_t>(std::uint32_t{ Alphabet[i >> 4] } << 8 | Alphabet[i & 0x0F]);
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				encodeTable[i] = value;
			}
			return encodeTable;
		}();

		template<std::size_t Count> requires (Count == 2)
		struct Encoded {

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[2];
		};

		static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

			return inSize << 1;
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto EncodeOneByte(auto in, auto* out) noexcept -> decltype(out) {

			auto a = std::bit_cast<std::uint8_t>(in[0]);

			auto encoded = std::bit_cast<Encoded<2>, std::uint16_t>(EncodeTable0<Alphabet>[a]);
			return encoded.Copy(out);
		}

		template<std::array<std::uint8_t, 16> const& Alphabet>
		static constexpr auto EncodeBytes(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			for (auto const end = in + inSize; in < end;) {

				auto a = std::bit_cast<std::uint8_t>(in[0]);
				in += 1;

				auto encoded = std::bit_cast<Encoded<2>, std::uint16_t>(EncodeTable0<Alphabet>[a]);
				out = encoded.Copy(out);
			}

			return out;
		}
	};
}