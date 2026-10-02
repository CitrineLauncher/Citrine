#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base64Decoder {
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

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return nullptr;
			return DecodeBytes<Alphabet>(in, inSize, out);
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> bool {

			output.clear();
			return Decode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
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

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::string& output) -> bool {

			output.clear();
			return Decode<Alphabet, PaddingChar...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
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

		template<std::array<std::uint8_t, 64> const& Alphabet, std::uint8_t... PaddingChar>
			requires (sizeof...(PaddingChar) <= 1)
		static constexpr auto Validate(auto const* in, std::size_t inSize) noexcept -> bool {

			if constexpr (sizeof...(PaddingChar) > 0)
				inSize = Unpad<PaddingChar...>(in, inSize);

			if (!ValidateInputSize(inSize))
				return false;

			return ValidateBytes<Alphabet>(in, inSize);
		}

	private:

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto DecodeTable0 = [] {

			auto decodeTable = std::array<std::uint32_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFF);
			for (auto i = 0uz; i < 64; ++i) {

				auto value = static_cast<std::uint32_t>(i) << 26;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto DecodeTable1 = [] {

			auto decodeTable = std::array<std::uint32_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFF);
			for (auto i = 0uz; i < 64; ++i) {

				auto value = static_cast<std::uint32_t>(i) << 20;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto DecodeTable2 = [] {

			auto decodeTable = std::array<std::uint32_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFF);
			for (auto i = 0uz; i < 64; ++i) {

				auto value = static_cast<std::uint32_t>(i) << 14;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto DecodeTable3 = [] {

			auto decodeTable = std::array<std::uint32_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFFFFFF);
			for (auto i = 0uz; i < 64; ++i) {

				auto value = static_cast<std::uint32_t>(i) << 8;
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				decodeTable[Alphabet[i]] = value;
			}
			return decodeTable;
		}();

		template<std::size_t Count> requires (Count >= 1 && Count <= 3)
		struct Decoded {

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			constexpr auto CopyPadded(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + 4, out);
				return out + Count;
			}

			constexpr explicit operator bool() const noexcept {

				return std::bit_cast<std::uint32_t>(Data) != 0xFFFFFFFF;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[4];
		};

		template<std::uint8_t PaddingChar>
		static constexpr auto Unpad(auto const* in, std::size_t inSize) noexcept -> std::size_t {

			auto const end = in + inSize;
			if (inSize >= 4 && inSize % 4 == 0 && end[-1] == PaddingChar)
				return inSize - 1 - static_cast<std::size_t>(end[-2] == PaddingChar);
			return inSize;
		}

		static constexpr auto ValidateInputSize(std::size_t inSize) noexcept -> bool {

			return (inSize % 4) != 1;
		}

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto ValidateBytes(auto const* in, std::size_t inSize) noexcept -> bool {

			auto const quadruples = inSize / 4;
			auto const remaining = inSize % 4;

			for (auto i = 0uz; i < quadruples; ++i) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				in += 4;

				auto const decoded = std::bit_cast<Decoded<3>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d]);
				if (!decoded) return false;
			}

			if (remaining == 3) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);

				auto const decoded = std::bit_cast<Decoded<2>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c]);
				if (!decoded) return false;
			}
			else if (remaining == 2) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);

				auto const decoded = std::bit_cast<Decoded<1>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b]);
				if (!decoded) return false;
			}

			return true;
		}

		static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

			return (inSize * 3) / 4;
		}

		template<std::array<std::uint8_t, 64> const& Alphabet>
		static constexpr auto DecodeBytes(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			if (inSize == 0) return out;
			auto quadruples = inSize / 4;
			auto remaining = inSize % 4;
			if (remaining == 0) { --quadruples; remaining = 4; }

			for (auto i = 0uz; i < quadruples; ++i) {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);
				in += 4;

				auto const decoded = std::bit_cast<Decoded<3>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d]);
				if (!decoded) return nullptr;
				out = decoded.CopyPadded(out);
			}

			switch (remaining) {
			case 4: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);
				auto const d = std::bit_cast<std::uint8_t>(in[3]);

				auto const decoded = std::bit_cast<Decoded<3>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c] | DecodeTable3<Alphabet>[d]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			} break;
			case 3: {

				auto const a = std::bit_cast<std::uint8_t>(in[0]);
				auto const b = std::bit_cast<std::uint8_t>(in[1]);
				auto const c = std::bit_cast<std::uint8_t>(in[2]);

				auto const decoded = std::bit_cast<Decoded<2>>(DecodeTable0<Alphabet>[a] | DecodeTable1<Alphabet>[b] | DecodeTable2<Alphabet>[c]);
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