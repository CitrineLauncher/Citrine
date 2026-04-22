#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base64Encoder {
	public:

		template<std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return CalculateOutputSize<PaddingChar...>(inSize);
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			return EncodeBytes<Alphabet, PaddingChar...>(in, inSize, out);
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> void {

			output.clear();
			Encode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> void {

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize<PaddingChar...>(inSize);

			output->resize(newSize);
			EncodeBytes<Alphabet, PaddingChar...>(in, inSize, output->data() + oldSize);
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::string& output) -> void {

			output.clear();
			Encode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, AppendTo<std::string> output) -> void {

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize<PaddingChar...>(inSize);

			output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

				EncodeBytes<Alphabet, PaddingChar...>(in, inSize, data + oldSize);
				return size;
			});
		}

	private:

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto EncodeTable0 = [] static {

			auto encodeTable = std::array<std::uint16_t, 4096>{};
			for (auto i = 0uz; i < encodeTable.size(); ++i) {

				auto value = static_cast<std::uint16_t>(std::uint32_t{ Alphabet[i >> 6] } << 8 | Alphabet[i & 0x3F]);
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				encodeTable[i] = value;
			}
			return encodeTable;
		}();

		template<std::size_t Count> requires (Count >= 1 && Count <= 2)
		struct Encoded {

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[2];
		};

		template<std::uint8_t PaddingChar, std::size_t Count> requires (Count >= 1 && Count <= 2)
		struct Padding {

			consteval Padding() noexcept {

				std::ranges::fill(Data, PaddingChar);
			}

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[Count];
		};

		template<std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

			if constexpr (sizeof...(PaddingChar) > 0)
				return ((inSize + 2) / 3) * 4;
			else
				return ((inSize * 4) | 2) / 3;
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto EncodeBytes(auto* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			auto const triples = inSize / 3;
			auto const remaining = inSize % 3;

			for (auto i = 0uz; i < triples; ++i) {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };
				auto const c = std::size_t{ std::bit_cast<std::uint8_t>(in[2]) };
				in += 3;

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 4) | (b >> 4)]);
				auto const encoded1 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((b & 0xF) << 8) | c]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);
			}

			switch (remaining) {
			case 2: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 4) | (b >> 4)]);
				auto const encoded1 = std::bit_cast<Encoded<1>>(EncodeTable0<Alphabet>[(b & 0xF) << 8]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 1>{}.Copy(out);
			} break;
			case 1: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };

				auto encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[a << 4]);
				out = encoded0.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 2>{}.Copy(out);
			} break;
			}

			return out;
		}
	};
}