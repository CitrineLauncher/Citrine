#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base32Encoder {
	public:

		template<std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return CalculateOutputSize<PaddingChar...>(inSize);
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			return EncodeBytes<Alphabet, PaddingChar...>(in, inSize, out);
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> void {

			output.clear();
			Encode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> void {

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize<PaddingChar...>(inSize);

			output->resize(newSize);
			EncodeBytes<Alphabet, PaddingChar...>(in, inSize, output->data() + oldSize);
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Encode(auto const* in, std::size_t inSize, std::string& output) -> void {

			output.clear();
			Encode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
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

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto EncodeTable0 = [] static {

			auto encodeTable = std::array<std::uint16_t, 1024>{};
			for (auto i = 0uz; i < encodeTable.size(); ++i) {

				auto value = static_cast<std::uint16_t>(std::uint32_t{ Alphabet[i >> 5] } << 8 | Alphabet[i & 0x1F]);
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

		template<std::uint8_t PaddingChar, std::size_t Count> requires (Count >= 1 && Count != 2 && Count != 5 && Count <= 6)
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
				return ((inSize + 4) / 5) * 8;
			else
				return ((inSize * 8) | 4) / 5;
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto EncodeBytes(auto* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			auto const quintuples = inSize / 5;
			auto const remaining = inSize % 5;

			for (auto i = 0uz; i < quintuples; ++i) {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };
				auto const c = std::size_t{ std::bit_cast<std::uint8_t>(in[2]) };
				auto const d = std::size_t{ std::bit_cast<std::uint8_t>(in[3]) };
				auto const e = std::size_t{ std::bit_cast<std::uint8_t>(in[4]) };
				in += 5;

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 2) | (b >> 6)]);
				auto const encoded1 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((b & 0x3F) << 4) | (c >> 4)]);
				auto const encoded2 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((c & 0xF) << 6) | (d >> 2)]);
				auto const encoded3 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((d & 0x3) << 8) | e]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);
				out = encoded2.Copy(out);
				out = encoded3.Copy(out);
			}

			switch (remaining) {
			case 4: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };
				auto const c = std::size_t{ std::bit_cast<std::uint8_t>(in[2]) };
				auto const d = std::size_t{ std::bit_cast<std::uint8_t>(in[3]) };

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 2) | (b >> 6)]);
				auto const encoded1 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((b & 0x3F) << 4) | (c >> 4)]);
				auto const encoded2 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((c & 0xF) << 6) | (d >> 2)]);
				auto const encoded3 = std::bit_cast<Encoded<1>>(EncodeTable0<Alphabet>[((d & 0x3) << 8)]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);
				out = encoded2.Copy(out);
				out = encoded3.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 1>{}.Copy(out);
			} break;
			case 3: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };
				auto const c = std::size_t{ std::bit_cast<std::uint8_t>(in[2]) };

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 2) | (b >> 6)]);
				auto const encoded1 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((b & 0x3F) << 4) | (c >> 4)]);
				auto const encoded2 = std::bit_cast<Encoded<1>>(EncodeTable0<Alphabet>[((c & 0xF) << 6)]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);
				out = encoded2.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 3>{}.Copy(out);
			} break;
			case 2: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };
				auto const b = std::size_t{ std::bit_cast<std::uint8_t>(in[1]) };

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 2) | (b >> 6)]);
				auto const encoded1 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[((b & 0x3F) << 4)]);
				out = encoded0.Copy(out);
				out = encoded1.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 4>{}.Copy(out);
			} break;
			case 1: {

				auto const a = std::size_t{ std::bit_cast<std::uint8_t>(in[0]) };

				auto const encoded0 = std::bit_cast<Encoded<2>>(EncodeTable0<Alphabet>[(a << 2)]);
				out = encoded0.Copy(out);

				if constexpr (sizeof...(PaddingChar) > 0)
					out = Padding<PaddingChar..., 6>{}.Copy(out);
			} break;
			}

			return out;
		}
	};
}