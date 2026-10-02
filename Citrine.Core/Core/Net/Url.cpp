#include "pch.h"
#include "Url.h"

#include "Core/Util/Ascii.h"
#include "Core/Util/Frozen.h"
#include "Core/Util/StringLiteral.h"
#include "Core/Util/ParseInteger.h"
#include "Core/Unicode/Utf.h"
#include "Core/Codec/UrlCodec.h"

#include <algorithm>
#include <array>
#include <bit>

using namespace Citrine;

namespace {

	static auto DefaultPortForScheme(std::string_view scheme) noexcept -> std::uint16_t {

		struct SchemeComparer {

			static constexpr auto operator()(std::string_view left, std::string_view right) noexcept -> bool {

				constexpr auto toLower = [](char ch) static { return Ascii::ToLower(ch); };

				return std::ranges::lexicographical_compare(left, right, {}, toLower, toLower);
			}
		};

		// Mapping from https://gist.github.com/mahmoud/2fe281a8daaff26cfe9c15d2c5bf5c8b
		static constexpr auto defaultPorts = MakeFrozenMap<std::string_view, std::uint16_t, SchemeComparer>({

			{ "acap", 674 },
			{ "afp", 548 },
			{ "dict", 2628 },
			{ "dns", 53 },
			{ "ftp", 21 },
			{ "git", 9418 },
			{ "gopher", 70 },
			{ "http", 80 },
			{ "https", 443 },
			{ "imap", 143 },
			{ "ipp", 631 },
			{ "ipps", 631 },
			{ "irc", 194 },
			{ "ircs", 6697 },
			{ "ldap", 389 },
			{ "ldaps", 636 },
			{ "mms", 1755 },
			{ "msrp", 2855 },
			{ "mtqp", 1038 },
			{ "nfs", 111 },
			{ "nntp", 119 },
			{ "nntps", 563 },
			{ "pop", 110 },
			{ "prospero", 1525 },
			{ "redis", 6379 },
			{ "rsync", 873 },
			{ "rtsp", 554 },
			{ "rtsps", 322 },
			{ "rtspu", 5005 },
			{ "sftp", 22 },
			{ "smb", 445 },
			{ "snmp", 161 },
			{ "ssh", 22 },
			{ "svn", 3690 },
			{ "telnet", 23 },
			{ "ventrilo", 3784 },
			{ "vnc", 5900 },
			{ "wais", 210 },
			{ "ws", 80 },
			{ "wss", 443 }
		});

		auto it = defaultPorts.find(scheme);
		return it != defaultPorts.end() ? it->second : 0;
	}
}

namespace Citrine {

	auto UrlComponentBase::Parse(std::string_view url, Components& components) noexcept -> bool {

		if (url.empty() || url.size() > 0xFFFF)
			return false;

		auto const begin = url.data();
		auto const end = begin + url.size();

		auto it = begin;

		constexpr auto isAlpha = AsciiMatcher<"A-Za-z">{};
		constexpr auto isDec = [](char ch) static { return DigitFromChar(ch) < 10; };
		constexpr auto isHex = [](char ch) static { return DigitFromChar(ch) < 16; };

		enum { SkipEscapeSequences };
		auto advanceWhile = [&]<typename... SkipEscapeSeq>(auto pred, SkipEscapeSeq...) {

			while (it < end) {

				if (pred(*it)) {
					
					++it;
					continue;
				}

				if constexpr (sizeof...(SkipEscapeSeq) > 0) {

					if ((end - it >= 3) && (it[0] == '%') && isHex(it[1]) && isHex(it[2])) {

						it += 3;
						continue;
					}
				}
				break;
			}
		};

		// Scheme
		{
			auto& scheme = components.Scheme;
			constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z+\\-.">{};

			if (!isAlpha(*it++))
				return false;

			advanceWhile(isValidChar);
			scheme.Size = static_cast<std::uint16_t>(it - begin);
		}

		if (end - it < 3 || std::string_view{ it, 3 } != "://")
			return false;
		it += 3;

		// Authority
		{
			auto parseComponentPair = [&] {

				auto first = Component{};
				auto second = Component{};
				constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$&'()*+,;=">{};
				
				first.Position = static_cast<std::uint16_t>(it - begin);
				advanceWhile(isValidChar, SkipEscapeSequences);
				first.Size = static_cast<std::uint16_t>(it - begin - first.Position);

				if (it == end || *it != ':')
					return std::pair{ first, second };
				++it;

				second.Position = static_cast<std::uint16_t>(it - begin);
				advanceWhile(isValidChar, SkipEscapeSequences);
				second.Size = static_cast<std::uint16_t>(it - begin - second.Position);

				return std::pair{ first, second };
			};

			auto componentPair = parseComponentPair();
			if (it < end && *it == '@') {

				++it;

				auto& [user, password] = componentPair;
				components.User = componentPair.first;
				components.Password = componentPair.second;

				componentPair = parseComponentPair();
			}

			auto& [host, port] = componentPair;
			if (host.Size == 0 && end - it >= 4 && *it == '[') {

				host.Position = static_cast<std::uint16_t>(it - begin);
				++it;

				if (*it == 'V' || *it == 'v') {

					++it;
					constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$&'()*+,;=:">{};

					if (!isHex(*it++))
						return false;
					advanceWhile(isHex);

					if (end - it < 2 || *it++ != '.' || !isValidChar(*it++))
						return false;
					advanceWhile(isValidChar);
				}
				else {

					auto totalBits = 0;
					auto ellipsisFound = false;
					auto ipv4Found = false;

					if (std::string_view{ it, 2 } == "::") {

						it += 2;
						totalBits = 16;
						ellipsisFound = true;
					}
					
					auto oldPos = it;
					advanceWhile(isHex);

					if (it == oldPos) {

						if (ellipsisFound)
							totalBits = 128 - 16; // The following loop will increment the count by 16
						else
							return false;
					}

					while (true) {

						if (it - oldPos > 4)
							return false;

						if ((totalBits += 16) == 128)
							break;

						if (end - it < 2)
							break;

						if (*it != ':') {

							ipv4Found = *it == '.';
							break;
						}
						++it;

						auto leadedByEllipsis = false;
						if (*it == ':') {

							++it;

							if (ellipsisFound)
								return false;

							if ((totalBits += 16) == 128)
								break;

							leadedByEllipsis = true;
							ellipsisFound = true;
						}

						oldPos = it;
						advanceWhile(isHex);

						if (it - oldPos == 0) {

							if (leadedByEllipsis)
								break;
							return false;
						}
					}

					if (ipv4Found) {

						it = oldPos;
						auto validateSeg = [&] {

							oldPos = it;
							auto num = std::uint32_t{};

							for (auto i = 0; i < 3; ++i) {

								if (it == end)
									break;

								auto digit = DigitFromChar(*it);
								if (digit >= 10)
									break;

								++it;
								num = num * 10 + static_cast<std::uint32_t>(digit);
							}
							return (it - oldPos) > 0 && num <= 0xFF;
						};

						if (!validateSeg())
							return false;
						if (it == end || *it++ != '.')
							return false;
						if (!validateSeg())
							return false;
						if (it == end || *it++ != '.')
							return false;
						if (!validateSeg())
							return false;
						if (it == end || *it++ != '.')
							return false;
						if (!validateSeg())
							return false;

						totalBits += 16; // The count was already incremented by 16 while matching an ipv6 segment
					}

					if (totalBits != 128 && !ellipsisFound)
						return false;
				}

				if (it == end || *it++ != ']')
					return false;
				host.Size = static_cast<std::uint16_t>(it - begin - host.Position);

				if (it < end && *it == ':') {

					++it;

					port.Position = static_cast<std::uint16_t>(it - begin);
					advanceWhile(isDec, SkipEscapeSequences);
					port.Size = static_cast<std::uint16_t>(it - begin - port.Position);
				}
			}

			components.Host = host;
			if (port.Size > 0) {

				auto& [pos, size] = port;
				if (!ParseInteger(begin + pos, begin + pos + size, components.Port))
					return false;
			}
			else {

				auto& scheme = components.Scheme;
				components.Port = DefaultPortForScheme({ begin, scheme.Size });
			}
		}

		// Path
		if (it < end && *it == '/') {

			auto& path = components.Path;
			constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$&'()*+,;=:@/">{};

			path.Position = static_cast<std::uint16_t>(it - begin);
			advanceWhile(isValidChar, SkipEscapeSequences);
			path.Size = static_cast<std::uint16_t>(it - begin - path.Position);
		}

		// Query
		if (it < end && *it == '?') {

			++it;
			auto& query = components.Query;
			constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$&'()*+,;=:@/?">{};

			query.Position = static_cast<std::uint16_t>(it - begin);
			advanceWhile(isValidChar, SkipEscapeSequences);
			query.Size = static_cast<std::uint16_t>(it - begin - query.Position);
		}

		// Fragment
		if (it < end && *it == '#') {

			++it;
			auto& fragment = components.Fragment;
			constexpr auto isValidChar = AsciiMatcher<"0-9A-Za-z\\-._~!$&'()*+,;=:@/?">{};

			fragment.Position = static_cast<std::uint16_t>(it - begin);
			advanceWhile(isValidChar, SkipEscapeSequences);
			fragment.Size = static_cast<std::uint16_t>(it - begin - fragment.Position);
		}

		return it == end;
	}

	template<typename StringType>
	auto UrlBase<StringType>::Scheme() const noexcept(NoThrow) -> StringType {

		auto scheme = components.Scheme;
		return { rawUrl.data() + scheme.Position, scheme.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::User() const noexcept(NoThrow) -> StringType {

		auto user = components.User;
		return { rawUrl.data() + user.Position, user.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::Password() const noexcept(NoThrow) -> StringType {

		auto password = components.Password;
		return { rawUrl.data() + password.Position, password.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::Host() const noexcept(NoThrow) -> StringType {

		auto host = components.Host;
		return { rawUrl.data() + host.Position, host.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::Port() const noexcept -> std::uint16_t {

		auto port = components.Port;
		return port;
	}

	template<typename StringType>
	auto UrlBase<StringType>::Path() const noexcept(NoThrow) -> StringType {

		auto path = components.Path;
		return { rawUrl.data() + path.Position, path.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::PathAndQuery() const noexcept(NoThrow) -> StringType {

		auto path = components.Path;
		auto query = components.Query;

		auto startPos = 0uz;
		auto combinedSize = 0uz;

		if (path.Size > 0) {

			startPos = path.Position;
			combinedSize = path.Size;
		}

		if (query.Size > 0) {

			if (startPos == 0)
				startPos = (query.Position - 1);
			combinedSize += (query.Size + 1);
		}

		return { rawUrl.data() + startPos, combinedSize };
	}

	template<typename StringType>
	auto UrlBase<StringType>::Query() const noexcept(NoThrow) -> StringType {

		auto query = components.Query;
		return { rawUrl.data() + query.Position, query.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::Fragment() const noexcept(NoThrow) -> StringType {

		auto fragment = components.Fragment;
		return { rawUrl.data() + fragment.Position, fragment.Size };
	}

	template<typename StringType>
	auto UrlBase<StringType>::IsEmpty() const noexcept -> bool {

		return rawUrl.empty();
	}

	template<typename StringType>
	auto UrlBase<StringType>::IsWellFormed() const noexcept -> bool {

		return wellFormed;
	}

	template<typename StringType>
	auto UrlBase<StringType>::Reset() noexcept -> void {

		rawUrl = {};
		components = {};
		wellFormed = {};
	}

	template class UrlBase<std::string>;
	template class UrlBase<std::string_view>;

	Url::Url(UrlView const& url)

		: UrlBase(url.rawUrl, url.components, url.wellFormed)
	{}

	Url::Url(Url&& other) noexcept
	
		: UrlBase(std::move(other.rawUrl), other.components, other.wellFormed)
	{
		other.Reset();
	}

	auto Url::operator=(Url&& other) noexcept -> Url& {

		if (this != &other) {

			rawUrl = std::move(other.rawUrl);
			components = other.components;
			wellFormed = other.wellFormed;

			other.Reset();
		}
		return *this;
	}

	auto Url::View() const& noexcept -> UrlView {

		return *this;
	}

	auto Url::RawUrl() const& noexcept -> std::string const& {

		return rawUrl;
	}

	auto Url::RawUrl() && noexcept -> std::string {

		auto value = std::move(rawUrl);
		Reset();
		return value;
	}

	auto Url::swap(Url& other) noexcept -> void {

		std::swap(rawUrl, other.rawUrl);
		std::swap(components, other.components);
		std::swap(wellFormed, other.wellFormed);
	}

	UrlView::UrlView(Url const& url) noexcept

		: UrlBase(url.rawUrl, url.components, url.wellFormed)
	{}

	auto UrlView::RawUrl() const noexcept -> std::string_view const& {

		return rawUrl;
	}

	auto UrlView::swap(UrlView& other) noexcept -> void {

		std::swap(rawUrl, other.rawUrl);
		std::swap(components, other.components);
		std::swap(wellFormed, other.wellFormed);
	}

	auto RawUrlReference::RawUrl() const noexcept -> std::string_view {

		return rawUrl;
	}

	RawUrlReference::operator std::string_view() const noexcept {

		return rawUrl;
	}

	auto UrlCombine(RawUrlReference baseUrl, std::filesystem::path const& path) -> Url {

		auto truncate = [](std::string_view url, bool removePath) static {

			static_assert(std::endian::native == std::endian::little);

			constexpr auto repeat8 = [](std::uint8_t value) static -> std::uint64_t {
				
				return 0x101010101010101 * value;
			};

			constexpr auto toU16 = [](char const* in) static -> std::uint16_t {

				auto value = std::uint16_t{};
				std::copy_n(in, 2, reinterpret_cast<std::uint8_t*>(&value));
				return value;
			};

			auto const begin = url.data();
			auto const end = begin + url.size();
			auto it = begin;

			auto truncatedSize = url.size();

			while (end - it >= 2) {

				if (toU16(it) == toU16("//")) {

					it += 2;
					break;
				}
				++it;
			}

			while (end - it >= 8) {

				auto swar = std::uint64_t{};
				std::copy_n(it, 8, reinterpret_cast<std::uint8_t*>(&swar));

				constexpr auto lo7Mask = repeat8(0x7f);
				auto const lo7 = swar & lo7Mask;

				auto const slash = (lo7 ^ repeat8('/')) + lo7Mask;
				auto const question = (lo7 ^ repeat8('?')) + lo7Mask;
				auto const hashtag = (lo7 ^ repeat8('#')) + lo7Mask;

				auto next = ~((slash & question & hashtag) | swar);
				next &= repeat8(0x80);

				if (next == 0) {

					it += 8;
					continue;
				}

				it += std::countr_zero(next) / 8;
				truncatedSize = it - begin;

				if (removePath || *it++ != '/') {

					it = end;
					break;
				}
			}

			while (it < end) {

				auto swar = std::uint64_t{};
				std::copy_n(it, end - it, reinterpret_cast<std::uint8_t*>(&swar));

				constexpr auto lo7Mask = repeat8(0x7f);
				auto const lo7 = swar & lo7Mask;

				auto const slash = (lo7 ^ repeat8('/')) + lo7Mask;
				auto const question = (lo7 ^ repeat8('?')) + lo7Mask;
				auto const hashtag = (lo7 ^ repeat8('#')) + lo7Mask;

				auto next = ~((slash & question & hashtag) | swar);
				next &= repeat8(0x80);

				if (next == 0)
					break;

				it += std::countr_zero(next) / 8;
				truncatedSize = it - begin;

				if (removePath || *it++ != '/')
					break;
			}

			return std::string_view{ begin, truncatedSize };
		};

		if (path.empty())
			return Url{ baseUrl };

		auto genericPath = ToUtf8(path.native());
		std::ranges::replace(genericPath, '\\', '/');
		auto pathIsAbsolute = genericPath.starts_with('/');

		auto truncatedUrl = truncate(baseUrl, pathIsAbsolute);
		if (truncatedUrl.ends_with('/'))
			truncatedUrl.remove_suffix(1);

		auto combinedUrl = std::string{ truncatedUrl };
		if (!pathIsAbsolute)
			combinedUrl.push_back('/');

		{
			auto it = genericPath.data();
			auto const end = it + genericPath.size();

			while (it < end) {

				auto first = it;
				it = std::find(it, end, '/');
				UrlCodec::Encode({ first, it }, AppendTo(combinedUrl));

				if (it == end)
					break;

				++it;
				combinedUrl.push_back('/');
			}
		}

		return Url{ std::move(combinedUrl) };
	}
}