#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base32Decoder {
	public:

		template<std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto DecodedSize(auto const* in, std::size_t inSize) noexcept -> std::size_t {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return 0;
			return CalculateOutputSize(inSize);
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return nullptr;
			return DecodeBytes<Alphabet>(in, inSize, out);
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> bool {

			output.clear();
			return Decode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> bool {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return false;

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize(newSize);
			if (!DecodeBytes<Alphabet>(in, inSize, output->data() + oldSize)) {

				output->resize(oldSize);
				return false;
			}
			return true;
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::string& output) -> bool {

			output.clear();
			return Decode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::string> output) -> bool {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return false;

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

				return DecodeBytes<Alphabet>(in, inSize, data + oldSize) ? size : oldSize;
			});
			return output->size() > oldSize || inSize == 0;
		}

		template<std::array<std::uint8_t, 32> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Validate(auto const* in, std::size_t inSize) noexcept -> bool {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return false;

			return ValidateBytes<Alphabet>(in, inSize);
		}

	private:

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable0 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 59;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable1 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 54;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable2 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 49;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable3 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 44;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable4 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 39;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable5 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 34;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable6 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 29;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeTable7 = [] {

			auto decodeTable = std::array<std::uint64_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFFFFFFFFFF);
			for (auto i = 0uz; i < 32; ++i) {

				auto value = static_cast<std::uint64_t>(i) << 24;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::size_t Count> requires (Count >= 1 && Count <= 5)
		struct Decoded {

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			constexpr auto CopyPadded(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + 8, out);
				return out + Count;
			}

			constexpr explicit operator bool() const noexcept {

				return std::bit_cast<std::uint64_t>(Data) != 0xFFFFFFFFFFFFFFFF;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[8];
		};

		template<std::uint8_t PaddingChar>
		static constexpr auto Unpad(auto const* in, std::size_t inSize) noexcept -> std::size_t {

			auto const end = in + inSize;
			if (inSize < 8 || inSize % 8 != 0 || end[-1] != PaddingChar)
				return inSize;

			if (end[-2] != PaddingChar)
				return inSize - 1;

			if (end[-3] != PaddingChar)
				return inSize - 2;

			if (end[-4] != PaddingChar)
				return inSize - 3;

			if (end[-5] != PaddingChar)
				return inSize - 4;

			if (end[-6] != PaddingChar)
				return inSize - 5;

			return inSize - 6;
		}

		static constexpr auto ValidateInputSize(std::size_t inSize) noexcept -> bool {

			return ((0b10110101 >> (inSize % 8)) & 1) != 0;
		}

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto ValidateBytes(auto const* in, std::size_t inSize) noexcept -> bool {

			auto const octuples = inSize / 8;
			auto const remaining = inSize % 8;

			for (auto i = 0uz; i < octuples; ++i) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);
				auto const f = std::bit_cast<std::uint8_t>(in[5]);
				auto const g = std::bit_cast<std::uint8_t>(in[6]);
				auto const h = std::bit_cast<std::uint8_t>(in[7]);
				in += 8;

				auto const decoded = std::bit_cast<Decoded<5>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e] | DecodeTable5<Alphabet>[f] | DecodeTable6<Alphabet>[g] | DecodeTable7<Alphabet>[h]);
				if (!decoded) return false;
			}

			switch (remaining) {
			case 7: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);
				auto const f = std::bit_cast<std::uint8_t>(in[5]);
				auto const g = std::bit_cast<std::uint8_t>(in[6]);

				auto const decoded = std::bit_cast<Decoded<4>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e] | DecodeTable5<Alphabet>[f] | DecodeTable6<Alphabet>[g]);
				if (!decoded) return false;
			} break;
			case 5: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);

				auto const decoded = std::bit_cast<Decoded<3>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e]);
				if (!decoded) return false;
			} break;
			case 4: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);

				auto const decoded = std::bit_cast<Decoded<2>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d]);
				if (!decoded) return false;
			} break;
			case 2: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);

				auto const decoded = std::bit_cast<Decoded<1>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b]);
				if (!decoded) return false;
			} break;
			}

			return true;
		}

		static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

			return (inSize * 5) / 8;
		}

		template<std::array<std::uint8_t, 32> const& Alphabet>
		static constexpr auto DecodeBytes(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			if (inSize == 0) return out;
			auto octuples = inSize / 8;
			auto remaining = inSize % 8;
			if (remaining < 5 && octuples > 0) { --octuples; remaining += 8; }

			for (auto i = 0uz; i < octuples; ++i) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);
				auto const f = std::bit_cast<std::uint8_t>(in[5]);
				auto const g = std::bit_cast<std::uint8_t>(in[6]);
				auto const h = std::bit_cast<std::uint8_t>(in[7]);
				in += 8;

				auto const decoded = std::bit_cast<Decoded<5>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e] | DecodeTable5<Alphabet>[f] | DecodeTable6<Alphabet>[g] | DecodeTable7<Alphabet>[h]);
				if (!decoded) return nullptr;
				out = decoded.CopyPadded(out);
			}

			if (remaining >= 8) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);
				auto const f = std::bit_cast<std::uint8_t>(in[5]);
				auto const g = std::bit_cast<std::uint8_t>(in[6]);
				auto const h = std::bit_cast<std::uint8_t>(in[7]);
				in += 8;

				auto const decoded = std::bit_cast<Decoded<5>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e] | DecodeTable5<Alphabet>[f] | DecodeTable6<Alphabet>[g] | DecodeTable7<Alphabet>[h]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);

				remaining -= 8;
			}

			switch (remaining) {
			case 7: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);
				auto const f = std::bit_cast<std::uint8_t>(in[5]);
				auto const g = std::bit_cast<std::uint8_t>(in[6]);

				auto const decoded = std::bit_cast<Decoded<4>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e] | DecodeTable5<Alphabet>[f] | DecodeTable6<Alphabet>[g]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			} break;
			case 5: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				auto const e = std::bit_cast<std::uint8_t>(in[4]);

				auto const decoded = std::bit_cast<Decoded<3>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d] | DecodeTable4<Alphabet>[e]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			} break;
			case 4: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);

				auto const decoded = std::bit_cast<Decoded<2>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			} break;
			case 2: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);

				auto const decoded = std::bit_cast<Decoded<1>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			} break;
			}

			return out;
		}
	};
}