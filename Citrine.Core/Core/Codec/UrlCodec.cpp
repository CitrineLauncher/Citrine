#include "pch.h"
#include "UrlCodec.h"

#include "Core/Codec/Base16.h"
#include "Core/Util/TrivialArray.h"

#include <algorithm>
#include <numeric>

using namespace Citrine;

namespace {

	template<UrlSpaceEncoding SpaceEncoding>
	constexpr auto EncodeTable = [] static { 

		auto table = std::array<char, 256>{};
		std::ranges::fill(table, '%');

		for (auto ch = 'A'; ch <= 'Z'; ++ch)
			table[ch] = ch;

		for (auto ch = 'a'; ch <= 'z'; ++ch)
			table[ch] = ch;

		for (auto ch = '0'; ch <= '9'; ++ch)
			table[ch] = ch;

		if constexpr (SpaceEncoding == UrlSpaceEncoding::PlusReplaced)
			table[' '] = '+';

		table['-'] = '-';
		table['.'] = '.';
		table['_'] = '_';
		table['~'] = '~';
		return table;
	}();
}

namespace Citrine {

	auto UrlCodec::Encode(std::string_view input, std::string& output, UrlSpaceEncoding spaceEncoding) -> void {

		output.clear();
		Encode(input, AppendTo(output), spaceEncoding);
	}

	auto UrlCodec::Encode(std::string_view input, AppendTo<std::string> output, UrlSpaceEncoding spaceEncoding) -> void {

		auto in = input.data();
		auto inSize = input.size();

		auto const& encodeTable = spaceEncoding == UrlSpaceEncoding::PlusReplaced
			? EncodeTable<UrlSpaceEncoding::PlusReplaced>
			: EncodeTable<UrlSpaceEncoding::PercentEscaped>;

		constexpr auto chunkSize = 256uz;
		auto buffer = TrivialArray<char, chunkSize * 3>{};

		for (auto chunks = inSize / chunkSize; chunks > 0; --chunks) {

			auto out = buffer.data();
			for (auto const end = in + chunkSize; in < end;) {

				auto ch = *in++;
				if ((*out++ = encodeTable[std::bit_cast<std::uint8_t>(ch)]) == '%')
					out = Base16::EncodeUpper(std::array{ ch }, out);
			}
			output->append(buffer.data(), out);
		}

		auto out = buffer.data();
		for (auto const end = in + inSize % chunkSize; in < end;) {

			auto ch = *in++;
			if ((*out++ = encodeTable[std::bit_cast<std::uint8_t>(ch)]) == '%')
				out = Base16::EncodeUpper(std::array{ ch }, out);
		}
		output->append(buffer.data(), out);
	}

	auto UrlCodec::Decode(std::string_view input, std::string& output, UrlSpaceEncoding spaceEncoding) -> bool {

		output.clear();
		return Decode(input, AppendTo(output), spaceEncoding);
	}

	auto UrlCodec::Decode(std::string_view input, AppendTo<std::string> output, UrlSpaceEncoding spaceEncoding) -> bool {

		auto in = input.data();
		auto inSize = input.size();

		auto const oldSize = output->size();
		auto const capacity = oldSize + inSize;

		auto const decodePlus = spaceEncoding == UrlSpaceEncoding::PlusReplaced;

		output->resize_and_overwrite(capacity, [&](char* data, std::size_t) -> std::size_t {

			auto out = data + oldSize;
			for (auto const end = in + inSize; in < end;) {

				auto ch = *in++;
				if (decodePlus && ch == '+') {

					ch = ' ';
				}
				else if (ch == '%') {

					if (end - in < 2 || !Base16::Decode(std::array{ in[0], in[1] }, &ch))
						return oldSize;
					in += 2;
				}
				*out++ = ch;
			}
			return out - data;
		});
		return output->size() > oldSize || inSize == 0;
	}

	auto UrlCodec::Validate(std::string_view input) noexcept -> bool {

		auto in = input.data();
		auto inSize = input.size();

		for (auto const end = in + inSize; in < end;) {

			if (*in++ == '%') {

				if (end - in < 2 || !Base16::Validate(std::array{ in[0], in[1] }))
					return false;
				in += 2;
			}
		}
		return true;
	}
}