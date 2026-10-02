#pragma once

#include "Base32Encoder.h"

#include <span>

namespace Citrine {

	class Base32Crockford {
	public:

		static constexpr auto UpperAlphabet = std::array<std::uint8_t, 32>{

			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
			'G', 'H', 'J', 'K', 'M', 'N', 'P', 'Q',
			'R', 'S', 'T', 'V', 'W', 'X', 'Y', 'Z'
		};
		static constexpr auto LowerAlphabet = std::array<std::uint8_t, 32>{

			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'j', 'k', 'm', 'n', 'p', 'q',
			'r', 's', 't', 'v', 'w', 'x', 'y', 'z'
		};

		static constexpr auto EncodedSize(std::size_t inSize) noexcept -> std::size_t {

			return Encoder::EncodedSize(inSize);
		}

		static constexpr auto EncodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Encoder::EncodedSize(input.size());
		}

		static constexpr auto EncodedSize(std::string_view input) noexcept -> std::size_t {

			return Encoder::EncodedSize(input.size());
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Encoder::Encode<UpperAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeUpper(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Encoder::Encode<UpperAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Encoder::Encode<LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto EncodeLower(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Encoder::Encode<LowerAlphabet>(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto DecodedSize(std::span<std::uint8_t const> input) noexcept -> std::size_t {

			return Decoder::DecodedSize(input.data(), input.size());
		}

		static constexpr auto DecodedSize(std::string_view input) noexcept -> std::size_t {

			return Decoder::DecodedSize(input.data(), input.size());
		}

		template<typename Output>
		static constexpr auto Decode(std::span<std::uint8_t const> input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Decoder::Decode(input.data(), input.size(), std::forward<Output>(output));
		}

		template<typename Output>
		static constexpr auto Decode(std::string_view input, Output&& output) noexcept(std::is_pointer_v<Output>) -> auto {

			return Decoder::Decode(input.data(), input.size(), std::forward<Output>(output));
		}

		static constexpr auto Validate(std::span<std::uint8_t const> input) noexcept -> bool {

			return Decoder::Validate(input.data(), input.size());
		}

		static constexpr auto Validate(std::string_view input) noexcept -> bool {

			return Decoder::Validate(input.data(), input.size());
		}

	private:

		using Encoder = Base32Encoder;

		struct Decoder {
		public:

			static constexpr auto DecodedSize(auto const* in, std::size_t inSize) noexcept -> std::size_t {

				auto actualSize = inSize - CountHyphens(in, inSize);
				if (!ValidateInputSize(actualSize))
					return 0;
				return CalculateOutputSize(actualSize);
			}

			static constexpr auto Decode(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

				auto actualSize = inSize - CountHyphens(in, inSize);
				if (!ValidateInputSize(actualSize))
					return nullptr;
				return DecodeBytes(in, inSize, out);
			}

			static constexpr auto Decode(auto const* in, std::size_t inSize, std::vector<std::uint8_t>& output) -> bool {

				output.clear();
				return Decode(in, inSize, AppendTo(output));
			}

			static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::vector<std::uint8_t>> output) -> bool {

				auto actualSize = inSize - CountHyphens(in, inSize);
				if (!ValidateInputSize(actualSize))
					return false;

				auto const oldSize = output->size();
				auto const newSize = oldSize + CalculateOutputSize(actualSize);

				output->resize(newSize);
				if (!DecodeBytes(in, inSize, output->data() + oldSize)) {

					output->resize(oldSize);
					return false;
				}
				return true;
			}

			static constexpr auto Decode(auto const* in, std::size_t inSize, std::string& output) -> bool {

				output.clear();
				return Decode(in, inSize, AppendTo(output));
			}

			static constexpr auto Decode(auto const* in, std::size_t inSize, AppendTo<std::string> output) -> bool {

				auto actualSize = inSize - CountHyphens(in, inSize);
				if (!ValidateInputSize(actualSize))
					return false;

				auto const oldSize = output->size();
				auto const newSize = oldSize + CalculateOutputSize(actualSize);

				output->resize_and_overwrite(newSize, [&](char* data, std::size_t size) {

					return DecodeBytes(in, inSize, data + oldSize) ? size : oldSize;
				});
				return output->size() > oldSize || inSize == 0;
			}

			static constexpr auto Validate(auto const* in, std::size_t inSize) noexcept -> bool {

				auto actualSize = inSize - CountHyphens(in, inSize);
				if (!ValidateInputSize(actualSize))
					return false;

				return ValidateBytes(in, inSize);
			}

		private:

			static constexpr auto DecodeTable = [] {

				auto table = std::array<std::int8_t, 256>{};
				std::ranges::fill(table, -1);

				for (auto i = 0uz; i < 32; ++i)
					table[UpperAlphabet[i]] = i;

				for (auto i = 0uz; i < 32; ++i)
					table[LowerAlphabet[i]] = i;

				for (auto ch : { 'o', 'O' })
					table[ch] = 0;

				for (auto ch : { 'i', 'I', 'l', 'L' })
					table[ch] = 1;

				return table;
			}();

			struct Octuple {

				explicit constexpr Octuple(auto* in) noexcept {

					std::copy(in, in + 8, Data);
				}

				constexpr auto ContainsHyphens() const noexcept -> bool {

					auto swar = std::bit_cast<std::uint64_t>(Data) ^ 0x2D2D2D2D2D2D2D2D;
					return static_cast<bool>((swar - 0x0101010101010101) & ~swar & 0x8080808080808080);
				}

				constexpr auto HyphenCount() const noexcept -> std::size_t {

					auto swar = std::bit_cast<std::uint64_t>(Data) ^ 0x2D2D2D2D2D2D2D2D;
					return std::popcount((swar - 0x0101010101010101) & ~swar & 0x8080808080808080);
				}

				static constexpr auto Size = 8;
				std::uint8_t Data[8];
			};

			template<std::size_t Count> requires (Count >= 1 && Count <= 5)
			struct Decoded {

				constexpr auto Copy(auto* out) const noexcept -> auto* {

					std::copy(Data, Data + Count, out);
					return out + Count;
				}

				static constexpr auto Size = Count;
				std::uint8_t Data[8];
			};

			static constexpr auto CountHyphens(auto const* in, std::size_t inSize) noexcept -> std::size_t {

				auto count = 0uz;
				auto const octuples = inSize / 8;
				auto const remaining = inSize % 8;

				for (auto i = 0uz; i < octuples; ++i) {

					count += Octuple{ in }.HyphenCount();
					in += 8;
				}

				switch (remaining) {
				case 7: count += static_cast<std::size_t>(in[6] == '-'); [[fallthrough]];
				case 6: count += static_cast<std::size_t>(in[5] == '-'); [[fallthrough]];
				case 5: count += static_cast<std::size_t>(in[4] == '-'); [[fallthrough]];
				case 4: count += static_cast<std::size_t>(in[3] == '-'); [[fallthrough]];
				case 3: count += static_cast<std::size_t>(in[2] == '-'); [[fallthrough]];
				case 2: count += static_cast<std::size_t>(in[1] == '-'); [[fallthrough]];
				case 1: count += static_cast<std::size_t>(in[0] == '-'); [[fallthrough]];
				case 0: break;
				default: std::unreachable();
				}

				return count;
			}

			static constexpr auto ValidateInputSize(std::size_t inSize) noexcept -> bool {

				return ((0b10110101 >> (inSize % 8)) & 1) != 0;
			}

			static constexpr auto ValidateBytes(auto const* in, std::size_t inSize) noexcept -> bool {

				auto const end = in + inSize;

				while (in < end) {

					auto ch = std::bit_cast<std::uint8_t>(*in++);
					if (ch == '-')
						continue;

					auto decodedBits = std::bit_cast<std::uint8_t>(DecodeTable[ch]);
					if (decodedBits == 0xFF)
						return false;
				}

				return true;
			}

			static constexpr auto CalculateOutputSize(std::size_t inSize) noexcept -> std::size_t {

				return (inSize * 5) / 8;
			}

			static constexpr auto DecodeBytes(auto const* in, std::size_t inSize, auto* out) noexcept -> decltype(out) {

				auto const end = in + inSize;

				auto buffer = std::uint64_t{};
				auto bufferedCount = 0;

				while (true) {

					if (end - in >= 8 && !Octuple{ in }.ContainsHyphens()) {

						auto const a = std::int64_t{ DecodeTable[in[0]] } << 35;
						auto const b = std::int64_t{ DecodeTable[in[1]] } << 30;
						auto const c = std::int64_t{ DecodeTable[in[2]] } << 25;
						auto const d = std::int64_t{ DecodeTable[in[3]] } << 20;
						auto const e = std::int64_t{ DecodeTable[in[4]] } << 15;
						auto const f = std::int64_t{ DecodeTable[in[5]] } << 10;
						auto const g = std::int64_t{ DecodeTable[in[6]] } << 5;
						auto const h = std::int64_t{ DecodeTable[in[7]] };
						in += 8;

						auto decodedChunk = a | b | c | d | e | f | g | h;
						if (decodedChunk < 0)
							return nullptr;

						buffer = static_cast<std::uint64_t>(decodedChunk) << 24;
						if constexpr (std::endian::native == std::endian::little)
							buffer = std::byteswap(buffer);

						out = std::bit_cast<Decoded<5>>(buffer).Copy(out);
						continue;
					}

					while (in < end) {

						auto ch = std::bit_cast<std::uint8_t>(*in++);
						if (ch == '-')
							continue;

						auto decodedBits = std::bit_cast<std::uint8_t>(DecodeTable[ch]);
						if (decodedBits == 0xFF)
							return nullptr;

						buffer <<= 5;
						buffer |= decodedBits;
						if (++bufferedCount == 8) {

							buffer <<= 24;
							if constexpr (std::endian::native == std::endian::little)
								buffer = std::byteswap(buffer);

							out = std::bit_cast<Decoded<5>>(buffer).Copy(out);
							bufferedCount = 0;
							break;
						}
					}

					if (end - in == 0)
						break;
				}

				if (bufferedCount > 0) {

					buffer <<= 24 + ((8 - bufferedCount) * 5);
					if constexpr (std::endian::native == std::endian::little)
						buffer = std::byteswap(buffer);

					switch ((bufferedCount * 5) / 8) {
					case 4: out = std::bit_cast<Decoded<4>>(buffer).Copy(out); break;
					case 3: out = std::bit_cast<Decoded<3>>(buffer).Copy(out); break;
					case 2: out = std::bit_cast<Decoded<2>>(buffer).Copy(out); break;
					case 1: out = std::bit_cast<Decoded<1>>(buffer).Copy(out); break;
					}
				}

				return out;
			}
		};
	};
}