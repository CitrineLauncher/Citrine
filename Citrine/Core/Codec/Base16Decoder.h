#pragma once

#include "Core/Util/Append.h"

#include <array>
#include <vector>
#include <string>
#include <bit>
#include <algorithm>

namespace Citrine {

	class Base16Decoder {
	public:

		static constexpr auto DecodedSize(std::size_t inSize) noexcept -> std::size_t {

			return ValidateInputSize(inSize) ? CalculateOutputSize(inSize) : 0;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto in, auto* out) noexcept -> decltype(out)
			requires(std::size(in) == 2)
		{
			return DecodeTwoBytes<Alphabets...>(in, out);
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			return ValidateInputSize(inSize) ? DecodeBytes<Alphabets...>(in, inSize, out) : nullptr;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> bool {

			output.clear();
			return Decode<Alphabets...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> bool {

			if (!ValidateInputSize(inSize))
				return false;

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize(newSize);
			if (!DecodeBytes<Alphabets...>(in, inSize, output->data() + oldSize)) {

				output->resize(oldSize);
				return false;
			}
			return true;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto const* in, std::size_t inSize, std::string& output) -> bool {

			output.clear();
			return Decode<Alphabets...>(in, inSize, AppendTo(output));
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::string> output) -> bool {

			if (!ValidateInputSize(inSize))
				return false;

			auto const oldSize = output->size();
			auto const newSize = oldSize + CalculateOutputSize(inSize);

			output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

				return DecodeBytes<Alphabets...>(in, inSize, data + oldSize) ? size : oldSize;
			});
			return output->size() > oldSize || inSize == 0;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Validate(auto in) noexcept -> bool
			requires(std::size(in) == 2)
		{
			return ValidateTwoBytes<Alphabets...>(in);
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto Validate(auto const* in, std::size_t inSize) noexcept -> bool {

			return ValidateInputSize(inSize) && ValidateBytes<Alphabets...>(in, inSize);
		}

	private:

		template<std::array<std::uint8_t, 16> const&... Alphabets> requires (sizeof...(Alphabets) >= 1)
		static constexpr auto DecodeTable0 = [] static {

			auto decodeTable = std::array<std::uint16_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFF);
			for (auto i = 0uz; i < 16; ++i) {

				auto value = static_cast<std::uint16_t>(static_cast<std::uint16_t>(i) << 12);
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				((decodeTable[Alphabets[i]] = value), ...);
			}
			return decodeTable;
		}();

		template<std::array<std::uint8_t, 16> const&... Alphabets> requires (sizeof...(Alphabets) >= 1)
		static constexpr auto DecodeTable1 = [] static {

			auto decodeTable = std::array<std::uint16_t, 256>{};
			std::ranges::fill(decodeTable, 0xFFFF);
			for (auto i = 0uz; i < 16; ++i) {

				auto value = static_cast<std::uint16_t>(static_cast<std::uint16_t>(i) << 8);
				if constexpr (std::endian::native == std::endian::little)
					value = std::byteswap(value);
				((decodeTable[Alphabets[i]] = value), ...);
			}
			return decodeTable;
		}();

		template<std::size_t Count> requires (Count == 1)
		struct Decoded {

			constexpr auto Copy(auto* out) const noexcept -> auto* {

				std::copy(Data, Data + Count, out);
				return out + Count;
			}

			constexpr explicit operator bool() const noexcept {

				return std::bit_cast<std::uint16_t>(Data) != 0xFFFF;
			}

			static constexpr auto Size = Count;
			std::uint8_t Data[2];
		};

		static constexpr auto ValidateInputSize(std::size_t inSize) noexcept -> bool {

			return (inSize & 1) == 0;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto ValidateTwoBytes(auto in) noexcept -> bool {

			auto a = std::bit_cast<std::uint8_t>(in[0]);
			auto b = std::bit_cast<std::uint8_t>(in[1]);

			auto decoded = std::bit_cast<Decoded<1>, std::uint16_t>(DecodeTable0<Alphabets...>[a] | DecodeTable1<Alphabets...>[b]);
			return bool{ decoded };
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto ValidateBytes(auto const* in, std::size_t inSize) noexcept -> bool {

			for (auto const end = in + inSize; in < end;) {

				auto a = std::bit_cast<std::uint8_t>(in[0]);
				auto b = std::bit_cast<std::uint8_t>(in[1]);
				in += 2;

				auto decoded = std::bit_cast<Decoded<1>, std::uint16_t>(DecodeTable0<Alphabets...>[a] | DecodeTable1<Alphabets...>[b]);
				if (!decoded) return false;
			}
			return true;
		}

		static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

			return inSize >> 1;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto DecodeTwoBytes(auto in, auto* out) noexcept -> decltype(out) {

			auto a = std::bit_cast<std::uint8_t>(in[0]);
			auto b = std::bit_cast<std::uint8_t>(in[1]);

			auto decoded = std::bit_cast<Decoded<1>, std::uint16_t>(DecodeTable0<Alphabets...>[a] | DecodeTable1<Alphabets...>[b]);
			return decoded ? decoded.Copy(out) : nullptr;
		}

		template<std::array<std::uint8_t, 16> const&... Alphabets>
		static constexpr auto DecodeBytes(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

			for (auto const end = in + inSize; in < end;) {

				auto a = std::bit_cast<std::uint8_t>(in[0]);
				auto b = std::bit_cast<std::uint8_t>(in[1]);
				in += 2;

				auto decoded = std::bit_cast<Decoded<1>, std::uint16_t>(DecodeTable0<Alphabets...>[a] | DecodeTable1<Alphabets...>[b]);
				if (!decoded) return nullptr;
				out = decoded.Copy(out);
			}

			return out;
		}
	};
}