#include "pch.h"
#include "Unicode.h"

#include <array>
#include <type_traits>
#include <numeric>

static_assert(sizeof(wchar_t) == sizeof(char16_t));
static_assert(std::is_unsigned_v<wchar_t>);

namespace {

	constexpr std::uint32_t GenericSurrogateMask = 0xF800;
	constexpr std::uint32_t GenericSurrogateValue = 0xD800;

	constexpr std::uint32_t SurrogateMask = 0xFC00;
	constexpr std::uint32_t HighSurrogateValue = 0xD800;
	constexpr std::uint32_t LowSurrogateValue = 0xDC00;

	constexpr std::uint32_t SurrogateCodepointOffset = 0x10000;
	constexpr std::uint32_t SurrogateCodepointMask = 0x03FF;
	constexpr std::uint32_t SurrogateCodepointBits = 10;

	constexpr auto Whitespaces = std::to_array<char16_t>({

		u'\u0009', u'\u000A', u'\u000B', u'\u000C', u'\u000D',
		u'\u0020', u'\u0085', u'\u00A0', u'\u1680', u'\u2000',
		u'\u2001', u'\u2002', u'\u2003', u'\u2004', u'\u2005',
		u'\u2006', u'\u2007', u'\u2008', u'\u2009', u'\u200A',
		u'\u2028', u'\u2029', u'\u202F', u'\u205F', u'\u3000'
	});

	// https://www.unicode.org/Public/16.0.0/ucd/CaseFolding.txt
	constexpr auto CaseFoldingTable0 = [] static {

		auto table = std::array<char16_t, 0x10000>{};
		std::ranges::iota(table, 0);

		for (auto ch : Whitespaces) {

			table[ch] = u'\u0020';
		}

		table[0x0041] = u'\u0061';
		table[0x0042] = u'\u0062';
		table[0x0043] = u'\u0063';
		table[0x0044] = u'\u0064';
		table[0x0045] = u'\u0065';
		table[0x0046] = u'\u0066';
		table[0x0047] = u'\u0067';
		table[0x0048] = u'\u0068';
		table[0x0049] = u'\u0069';
		table[0x004A] = u'\u006A';
		table[0x004B] = u'\u006B';
		table[0x004C] = u'\u006C';
		table[0x004D] = u'\u006D';
		table[0x004E] = u'\u006E';
		table[0x004F] = u'\u006F';
		table[0x0050] = u'\u0070';
		table[0x0051] = u'\u0071';
		table[0x0052] = u'\u0072';
		table[0x0053] = u'\u0073';
		table[0x0054] = u'\u0074';
		table[0x0055] = u'\u0075';
		table[0x0056] = u'\u0076';
		table[0x0057] = u'\u0077';
		table[0x0058] = u'\u0078';
		table[0x0059] = u'\u0079';
		table[0x005A] = u'\u007A';
		table[0x00B5] = u'\u03BC';
		table[0x00C0] = u'\u00E0';
		table[0x00C1] = u'\u00E1';
		table[0x00C2] = u'\u00E2';
		table[0x00C3] = u'\u00E3';
		table[0x00C4] = u'\u00E4';
		table[0x00C5] = u'\u00E5';
		table[0x00C6] = u'\u00E6';
		table[0x00C7] = u'\u00E7';
		table[0x00C8] = u'\u00E8';
		table[0x00C9] = u'\u00E9';
		table[0x00CA] = u'\u00EA';
		table[0x00CB] = u'\u00EB';
		table[0x00CC] = u'\u00EC';
		table[0x00CD] = u'\u00ED';
		table[0x00CE] = u'\u00EE';
		table[0x00CF] = u'\u00EF';
		table[0x00D0] = u'\u00F0';
		table[0x00D1] = u'\u00F1';
		table[0x00D2] = u'\u00F2';
		table[0x00D3] = u'\u00F3';
		table[0x00D4] = u'\u00F4';
		table[0x00D5] = u'\u00F5';
		table[0x00D6] = u'\u00F6';
		table[0x00D8] = u'\u00F8';
		table[0x00D9] = u'\u00F9';
		table[0x00DA] = u'\u00FA';
		table[0x00DB] = u'\u00FB';
		table[0x00DC] = u'\u00FC';
		table[0x00DD] = u'\u00FD';
		table[0x00DE] = u'\u00FE';
		table[0x0100] = u'\u0101';
		table[0x0102] = u'\u0103';
		table[0x0104] = u'\u0105';
		table[0x0106] = u'\u0107';
		table[0x0108] = u'\u0109';
		table[0x010A] = u'\u010B';
		table[0x010C] = u'\u010D';
		table[0x010E] = u'\u010F';
		table[0x0110] = u'\u0111';
		table[0x0112] = u'\u0113';
		table[0x0114] = u'\u0115';
		table[0x0116] = u'\u0117';
		table[0x0118] = u'\u0119';
		table[0x011A] = u'\u011B';
		table[0x011C] = u'\u011D';
		table[0x011E] = u'\u011F';
		table[0x0120] = u'\u0121';
		table[0x0122] = u'\u0123';
		table[0x0124] = u'\u0125';
		table[0x0126] = u'\u0127';
		table[0x0128] = u'\u0129';
		table[0x012A] = u'\u012B';
		table[0x012C] = u'\u012D';
		table[0x012E] = u'\u012F';
		table[0x0132] = u'\u0133';
		table[0x0134] = u'\u0135';
		table[0x0136] = u'\u0137';
		table[0x0139] = u'\u013A';
		table[0x013B] = u'\u013C';
		table[0x013D] = u'\u013E';
		table[0x013F] = u'\u0140';
		table[0x0141] = u'\u0142';
		table[0x0143] = u'\u0144';
		table[0x0145] = u'\u0146';
		table[0x0147] = u'\u0148';
		table[0x014A] = u'\u014B';
		table[0x014C] = u'\u014D';
		table[0x014E] = u'\u014F';
		table[0x0150] = u'\u0151';
		table[0x0152] = u'\u0153';
		table[0x0154] = u'\u0155';
		table[0x0156] = u'\u0157';
		table[0x0158] = u'\u0159';
		table[0x015A] = u'\u015B';
		table[0x015C] = u'\u015D';
		table[0x015E] = u'\u015F';
		table[0x0160] = u'\u0161';
		table[0x0162] = u'\u0163';
		table[0x0164] = u'\u0165';
		table[0x0166] = u'\u0167';
		table[0x0168] = u'\u0169';
		table[0x016A] = u'\u016B';
		table[0x016C] = u'\u016D';
		table[0x016E] = u'\u016F';
		table[0x0170] = u'\u0171';
		table[0x0172] = u'\u0173';
		table[0x0174] = u'\u0175';
		table[0x0176] = u'\u0177';
		table[0x0178] = u'\u00FF';
		table[0x0179] = u'\u017A';
		table[0x017B] = u'\u017C';
		table[0x017D] = u'\u017E';
		table[0x017F] = u'\u0073';
		table[0x0181] = u'\u0253';
		table[0x0182] = u'\u0183';
		table[0x0184] = u'\u0185';
		table[0x0186] = u'\u0254';
		table[0x0187] = u'\u0188';
		table[0x0189] = u'\u0256';
		table[0x018A] = u'\u0257';
		table[0x018B] = u'\u018C';
		table[0x018E] = u'\u01DD';
		table[0x018F] = u'\u0259';
		table[0x0190] = u'\u025B';
		table[0x0191] = u'\u0192';
		table[0x0193] = u'\u0260';
		table[0x0194] = u'\u0263';
		table[0x0196] = u'\u0269';
		table[0x0197] = u'\u0268';
		table[0x0198] = u'\u0199';
		table[0x019C] = u'\u026F';
		table[0x019D] = u'\u0272';
		table[0x019F] = u'\u0275';
		table[0x01A0] = u'\u01A1';
		table[0x01A2] = u'\u01A3';
		table[0x01A4] = u'\u01A5';
		table[0x01A6] = u'\u0280';
		table[0x01A7] = u'\u01A8';
		table[0x01A9] = u'\u0283';
		table[0x01AC] = u'\u01AD';
		table[0x01AE] = u'\u0288';
		table[0x01AF] = u'\u01B0';
		table[0x01B1] = u'\u028A';
		table[0x01B2] = u'\u028B';
		table[0x01B3] = u'\u01B4';
		table[0x01B5] = u'\u01B6';
		table[0x01B7] = u'\u0292';
		table[0x01B8] = u'\u01B9';
		table[0x01BC] = u'\u01BD';
		table[0x01C4] = u'\u01C6';
		table[0x01C5] = u'\u01C6';
		table[0x01C7] = u'\u01C9';
		table[0x01C8] = u'\u01C9';
		table[0x01CA] = u'\u01CC';
		table[0x01CB] = u'\u01CC';
		table[0x01CD] = u'\u01CE';
		table[0x01CF] = u'\u01D0';
		table[0x01D1] = u'\u01D2';
		table[0x01D3] = u'\u01D4';
		table[0x01D5] = u'\u01D6';
		table[0x01D7] = u'\u01D8';
		table[0x01D9] = u'\u01DA';
		table[0x01DB] = u'\u01DC';
		table[0x01DE] = u'\u01DF';
		table[0x01E0] = u'\u01E1';
		table[0x01E2] = u'\u01E3';
		table[0x01E4] = u'\u01E5';
		table[0x01E6] = u'\u01E7';
		table[0x01E8] = u'\u01E9';
		table[0x01EA] = u'\u01EB';
		table[0x01EC] = u'\u01ED';
		table[0x01EE] = u'\u01EF';
		table[0x01F1] = u'\u01F3';
		table[0x01F2] = u'\u01F3';
		table[0x01F4] = u'\u01F5';
		table[0x01F6] = u'\u0195';
		table[0x01F7] = u'\u01BF';
		table[0x01F8] = u'\u01F9';
		table[0x01FA] = u'\u01FB';
		table[0x01FC] = u'\u01FD';
		table[0x01FE] = u'\u01FF';
		table[0x0200] = u'\u0201';
		table[0x0202] = u'\u0203';
		table[0x0204] = u'\u0205';
		table[0x0206] = u'\u0207';
		table[0x0208] = u'\u0209';
		table[0x020A] = u'\u020B';
		table[0x020C] = u'\u020D';
		table[0x020E] = u'\u020F';
		table[0x0210] = u'\u0211';
		table[0x0212] = u'\u0213';
		table[0x0214] = u'\u0215';
		table[0x0216] = u'\u0217';
		table[0x0218] = u'\u0219';
		table[0x021A] = u'\u021B';
		table[0x021C] = u'\u021D';
		table[0x021E] = u'\u021F';
		table[0x0220] = u'\u019E';
		table[0x0222] = u'\u0223';
		table[0x0224] = u'\u0225';
		table[0x0226] = u'\u0227';
		table[0x0228] = u'\u0229';
		table[0x022A] = u'\u022B';
		table[0x022C] = u'\u022D';
		table[0x022E] = u'\u022F';
		table[0x0230] = u'\u0231';
		table[0x0232] = u'\u0233';
		table[0x023A] = u'\u2C65';
		table[0x023B] = u'\u023C';
		table[0x023D] = u'\u019A';
		table[0x023E] = u'\u2C66';
		table[0x0241] = u'\u0242';
		table[0x0243] = u'\u0180';
		table[0x0244] = u'\u0289';
		table[0x0245] = u'\u028C';
		table[0x0246] = u'\u0247';
		table[0x0248] = u'\u0249';
		table[0x024A] = u'\u024B';
		table[0x024C] = u'\u024D';
		table[0x024E] = u'\u024F';
		table[0x0345] = u'\u03B9';
		table[0x0370] = u'\u0371';
		table[0x0372] = u'\u0373';
		table[0x0376] = u'\u0377';
		table[0x037F] = u'\u03F3';
		table[0x0386] = u'\u03AC';
		table[0x0388] = u'\u03AD';
		table[0x0389] = u'\u03AE';
		table[0x038A] = u'\u03AF';
		table[0x038C] = u'\u03CC';
		table[0x038E] = u'\u03CD';
		table[0x038F] = u'\u03CE';
		table[0x0391] = u'\u03B1';
		table[0x0392] = u'\u03B2';
		table[0x0393] = u'\u03B3';
		table[0x0394] = u'\u03B4';
		table[0x0395] = u'\u03B5';
		table[0x0396] = u'\u03B6';
		table[0x0397] = u'\u03B7';
		table[0x0398] = u'\u03B8';
		table[0x0399] = u'\u03B9';
		table[0x039A] = u'\u03BA';
		table[0x039B] = u'\u03BB';
		table[0x039C] = u'\u03BC';
		table[0x039D] = u'\u03BD';
		table[0x039E] = u'\u03BE';
		table[0x039F] = u'\u03BF';
		table[0x03A0] = u'\u03C0';
		table[0x03A1] = u'\u03C1';
		table[0x03A3] = u'\u03C3';
		table[0x03A4] = u'\u03C4';
		table[0x03A5] = u'\u03C5';
		table[0x03A6] = u'\u03C6';
		table[0x03A7] = u'\u03C7';
		table[0x03A8] = u'\u03C8';
		table[0x03A9] = u'\u03C9';
		table[0x03AA] = u'\u03CA';
		table[0x03AB] = u'\u03CB';
		table[0x03C2] = u'\u03C3';
		table[0x03CF] = u'\u03D7';
		table[0x03D0] = u'\u03B2';
		table[0x03D1] = u'\u03B8';
		table[0x03D5] = u'\u03C6';
		table[0x03D6] = u'\u03C0';
		table[0x03D8] = u'\u03D9';
		table[0x03DA] = u'\u03DB';
		table[0x03DC] = u'\u03DD';
		table[0x03DE] = u'\u03DF';
		table[0x03E0] = u'\u03E1';
		table[0x03E2] = u'\u03E3';
		table[0x03E4] = u'\u03E5';
		table[0x03E6] = u'\u03E7';
		table[0x03E8] = u'\u03E9';
		table[0x03EA] = u'\u03EB';
		table[0x03EC] = u'\u03ED';
		table[0x03EE] = u'\u03EF';
		table[0x03F0] = u'\u03BA';
		table[0x03F1] = u'\u03C1';
		table[0x03F4] = u'\u03B8';
		table[0x03F5] = u'\u03B5';
		table[0x03F7] = u'\u03F8';
		table[0x03F9] = u'\u03F2';
		table[0x03FA] = u'\u03FB';
		table[0x03FD] = u'\u037B';
		table[0x03FE] = u'\u037C';
		table[0x03FF] = u'\u037D';
		table[0x0400] = u'\u0450';
		table[0x0401] = u'\u0451';
		table[0x0402] = u'\u0452';
		table[0x0403] = u'\u0453';
		table[0x0404] = u'\u0454';
		table[0x0405] = u'\u0455';
		table[0x0406] = u'\u0456';
		table[0x0407] = u'\u0457';
		table[0x0408] = u'\u0458';
		table[0x0409] = u'\u0459';
		table[0x040A] = u'\u045A';
		table[0x040B] = u'\u045B';
		table[0x040C] = u'\u045C';
		table[0x040D] = u'\u045D';
		table[0x040E] = u'\u045E';
		table[0x040F] = u'\u045F';
		table[0x0410] = u'\u0430';
		table[0x0411] = u'\u0431';
		table[0x0412] = u'\u0432';
		table[0x0413] = u'\u0433';
		table[0x0414] = u'\u0434';
		table[0x0415] = u'\u0435';
		table[0x0416] = u'\u0436';
		table[0x0417] = u'\u0437';
		table[0x0418] = u'\u0438';
		table[0x0419] = u'\u0439';
		table[0x041A] = u'\u043A';
		table[0x041B] = u'\u043B';
		table[0x041C] = u'\u043C';
		table[0x041D] = u'\u043D';
		table[0x041E] = u'\u043E';
		table[0x041F] = u'\u043F';
		table[0x0420] = u'\u0440';
		table[0x0421] = u'\u0441';
		table[0x0422] = u'\u0442';
		table[0x0423] = u'\u0443';
		table[0x0424] = u'\u0444';
		table[0x0425] = u'\u0445';
		table[0x0426] = u'\u0446';
		table[0x0427] = u'\u0447';
		table[0x0428] = u'\u0448';
		table[0x0429] = u'\u0449';
		table[0x042A] = u'\u044A';
		table[0x042B] = u'\u044B';
		table[0x042C] = u'\u044C';
		table[0x042D] = u'\u044D';
		table[0x042E] = u'\u044E';
		table[0x042F] = u'\u044F';
		table[0x0460] = u'\u0461';
		table[0x0462] = u'\u0463';
		table[0x0464] = u'\u0465';
		table[0x0466] = u'\u0467';
		table[0x0468] = u'\u0469';
		table[0x046A] = u'\u046B';
		table[0x046C] = u'\u046D';
		table[0x046E] = u'\u046F';
		table[0x0470] = u'\u0471';
		table[0x0472] = u'\u0473';
		table[0x0474] = u'\u0475';
		table[0x0476] = u'\u0477';
		table[0x0478] = u'\u0479';
		table[0x047A] = u'\u047B';
		table[0x047C] = u'\u047D';
		table[0x047E] = u'\u047F';
		table[0x0480] = u'\u0481';
		table[0x048A] = u'\u048B';
		table[0x048C] = u'\u048D';
		table[0x048E] = u'\u048F';
		table[0x0490] = u'\u0491';
		table[0x0492] = u'\u0493';
		table[0x0494] = u'\u0495';
		table[0x0496] = u'\u0497';
		table[0x0498] = u'\u0499';
		table[0x049A] = u'\u049B';
		table[0x049C] = u'\u049D';
		table[0x049E] = u'\u049F';
		table[0x04A0] = u'\u04A1';
		table[0x04A2] = u'\u04A3';
		table[0x04A4] = u'\u04A5';
		table[0x04A6] = u'\u04A7';
		table[0x04A8] = u'\u04A9';
		table[0x04AA] = u'\u04AB';
		table[0x04AC] = u'\u04AD';
		table[0x04AE] = u'\u04AF';
		table[0x04B0] = u'\u04B1';
		table[0x04B2] = u'\u04B3';
		table[0x04B4] = u'\u04B5';
		table[0x04B6] = u'\u04B7';
		table[0x04B8] = u'\u04B9';
		table[0x04BA] = u'\u04BB';
		table[0x04BC] = u'\u04BD';
		table[0x04BE] = u'\u04BF';
		table[0x04C0] = u'\u04CF';
		table[0x04C1] = u'\u04C2';
		table[0x04C3] = u'\u04C4';
		table[0x04C5] = u'\u04C6';
		table[0x04C7] = u'\u04C8';
		table[0x04C9] = u'\u04CA';
		table[0x04CB] = u'\u04CC';
		table[0x04CD] = u'\u04CE';
		table[0x04D0] = u'\u04D1';
		table[0x04D2] = u'\u04D3';
		table[0x04D4] = u'\u04D5';
		table[0x04D6] = u'\u04D7';
		table[0x04D8] = u'\u04D9';
		table[0x04DA] = u'\u04DB';
		table[0x04DC] = u'\u04DD';
		table[0x04DE] = u'\u04DF';
		table[0x04E0] = u'\u04E1';
		table[0x04E2] = u'\u04E3';
		table[0x04E4] = u'\u04E5';
		table[0x04E6] = u'\u04E7';
		table[0x04E8] = u'\u04E9';
		table[0x04EA] = u'\u04EB';
		table[0x04EC] = u'\u04ED';
		table[0x04EE] = u'\u04EF';
		table[0x04F0] = u'\u04F1';
		table[0x04F2] = u'\u04F3';
		table[0x04F4] = u'\u04F5';
		table[0x04F6] = u'\u04F7';
		table[0x04F8] = u'\u04F9';
		table[0x04FA] = u'\u04FB';
		table[0x04FC] = u'\u04FD';
		table[0x04FE] = u'\u04FF';
		table[0x0500] = u'\u0501';
		table[0x0502] = u'\u0503';
		table[0x0504] = u'\u0505';
		table[0x0506] = u'\u0507';
		table[0x0508] = u'\u0509';
		table[0x050A] = u'\u050B';
		table[0x050C] = u'\u050D';
		table[0x050E] = u'\u050F';
		table[0x0510] = u'\u0511';
		table[0x0512] = u'\u0513';
		table[0x0514] = u'\u0515';
		table[0x0516] = u'\u0517';
		table[0x0518] = u'\u0519';
		table[0x051A] = u'\u051B';
		table[0x051C] = u'\u051D';
		table[0x051E] = u'\u051F';
		table[0x0520] = u'\u0521';
		table[0x0522] = u'\u0523';
		table[0x0524] = u'\u0525';
		table[0x0526] = u'\u0527';
		table[0x0528] = u'\u0529';
		table[0x052A] = u'\u052B';
		table[0x052C] = u'\u052D';
		table[0x052E] = u'\u052F';
		table[0x0531] = u'\u0561';
		table[0x0532] = u'\u0562';
		table[0x0533] = u'\u0563';
		table[0x0534] = u'\u0564';
		table[0x0535] = u'\u0565';
		table[0x0536] = u'\u0566';
		table[0x0537] = u'\u0567';
		table[0x0538] = u'\u0568';
		table[0x0539] = u'\u0569';
		table[0x053A] = u'\u056A';
		table[0x053B] = u'\u056B';
		table[0x053C] = u'\u056C';
		table[0x053D] = u'\u056D';
		table[0x053E] = u'\u056E';
		table[0x053F] = u'\u056F';
		table[0x0540] = u'\u0570';
		table[0x0541] = u'\u0571';
		table[0x0542] = u'\u0572';
		table[0x0543] = u'\u0573';
		table[0x0544] = u'\u0574';
		table[0x0545] = u'\u0575';
		table[0x0546] = u'\u0576';
		table[0x0547] = u'\u0577';
		table[0x0548] = u'\u0578';
		table[0x0549] = u'\u0579';
		table[0x054A] = u'\u057A';
		table[0x054B] = u'\u057B';
		table[0x054C] = u'\u057C';
		table[0x054D] = u'\u057D';
		table[0x054E] = u'\u057E';
		table[0x054F] = u'\u057F';
		table[0x0550] = u'\u0580';
		table[0x0551] = u'\u0581';
		table[0x0552] = u'\u0582';
		table[0x0553] = u'\u0583';
		table[0x0554] = u'\u0584';
		table[0x0555] = u'\u0585';
		table[0x0556] = u'\u0586';
		table[0x10A0] = u'\u2D00';
		table[0x10A1] = u'\u2D01';
		table[0x10A2] = u'\u2D02';
		table[0x10A3] = u'\u2D03';
		table[0x10A4] = u'\u2D04';
		table[0x10A5] = u'\u2D05';
		table[0x10A6] = u'\u2D06';
		table[0x10A7] = u'\u2D07';
		table[0x10A8] = u'\u2D08';
		table[0x10A9] = u'\u2D09';
		table[0x10AA] = u'\u2D0A';
		table[0x10AB] = u'\u2D0B';
		table[0x10AC] = u'\u2D0C';
		table[0x10AD] = u'\u2D0D';
		table[0x10AE] = u'\u2D0E';
		table[0x10AF] = u'\u2D0F';
		table[0x10B0] = u'\u2D10';
		table[0x10B1] = u'\u2D11';
		table[0x10B2] = u'\u2D12';
		table[0x10B3] = u'\u2D13';
		table[0x10B4] = u'\u2D14';
		table[0x10B5] = u'\u2D15';
		table[0x10B6] = u'\u2D16';
		table[0x10B7] = u'\u2D17';
		table[0x10B8] = u'\u2D18';
		table[0x10B9] = u'\u2D19';
		table[0x10BA] = u'\u2D1A';
		table[0x10BB] = u'\u2D1B';
		table[0x10BC] = u'\u2D1C';
		table[0x10BD] = u'\u2D1D';
		table[0x10BE] = u'\u2D1E';
		table[0x10BF] = u'\u2D1F';
		table[0x10C0] = u'\u2D20';
		table[0x10C1] = u'\u2D21';
		table[0x10C2] = u'\u2D22';
		table[0x10C3] = u'\u2D23';
		table[0x10C4] = u'\u2D24';
		table[0x10C5] = u'\u2D25';
		table[0x10C7] = u'\u2D27';
		table[0x10CD] = u'\u2D2D';
		table[0x13F8] = u'\u13F0';
		table[0x13F9] = u'\u13F1';
		table[0x13FA] = u'\u13F2';
		table[0x13FB] = u'\u13F3';
		table[0x13FC] = u'\u13F4';
		table[0x13FD] = u'\u13F5';
		table[0x1C80] = u'\u0432';
		table[0x1C81] = u'\u0434';
		table[0x1C82] = u'\u043E';
		table[0x1C83] = u'\u0441';
		table[0x1C84] = u'\u0442';
		table[0x1C85] = u'\u0442';
		table[0x1C86] = u'\u044A';
		table[0x1C87] = u'\u0463';
		table[0x1C88] = u'\uA64B';
		table[0x1C90] = u'\u10D0';
		table[0x1C91] = u'\u10D1';
		table[0x1C92] = u'\u10D2';
		table[0x1C93] = u'\u10D3';
		table[0x1C94] = u'\u10D4';
		table[0x1C95] = u'\u10D5';
		table[0x1C96] = u'\u10D6';
		table[0x1C97] = u'\u10D7';
		table[0x1C98] = u'\u10D8';
		table[0x1C99] = u'\u10D9';
		table[0x1C9A] = u'\u10DA';
		table[0x1C9B] = u'\u10DB';
		table[0x1C9C] = u'\u10DC';
		table[0x1C9D] = u'\u10DD';
		table[0x1C9E] = u'\u10DE';
		table[0x1C9F] = u'\u10DF';
		table[0x1CA0] = u'\u10E0';
		table[0x1CA1] = u'\u10E1';
		table[0x1CA2] = u'\u10E2';
		table[0x1CA3] = u'\u10E3';
		table[0x1CA4] = u'\u10E4';
		table[0x1CA5] = u'\u10E5';
		table[0x1CA6] = u'\u10E6';
		table[0x1CA7] = u'\u10E7';
		table[0x1CA8] = u'\u10E8';
		table[0x1CA9] = u'\u10E9';
		table[0x1CAA] = u'\u10EA';
		table[0x1CAB] = u'\u10EB';
		table[0x1CAC] = u'\u10EC';
		table[0x1CAD] = u'\u10ED';
		table[0x1CAE] = u'\u10EE';
		table[0x1CAF] = u'\u10EF';
		table[0x1CB0] = u'\u10F0';
		table[0x1CB1] = u'\u10F1';
		table[0x1CB2] = u'\u10F2';
		table[0x1CB3] = u'\u10F3';
		table[0x1CB4] = u'\u10F4';
		table[0x1CB5] = u'\u10F5';
		table[0x1CB6] = u'\u10F6';
		table[0x1CB7] = u'\u10F7';
		table[0x1CB8] = u'\u10F8';
		table[0x1CB9] = u'\u10F9';
		table[0x1CBA] = u'\u10FA';
		table[0x1CBD] = u'\u10FD';
		table[0x1CBE] = u'\u10FE';
		table[0x1CBF] = u'\u10FF';
		table[0x1E00] = u'\u1E01';
		table[0x1E02] = u'\u1E03';
		table[0x1E04] = u'\u1E05';
		table[0x1E06] = u'\u1E07';
		table[0x1E08] = u'\u1E09';
		table[0x1E0A] = u'\u1E0B';
		table[0x1E0C] = u'\u1E0D';
		table[0x1E0E] = u'\u1E0F';
		table[0x1E10] = u'\u1E11';
		table[0x1E12] = u'\u1E13';
		table[0x1E14] = u'\u1E15';
		table[0x1E16] = u'\u1E17';
		table[0x1E18] = u'\u1E19';
		table[0x1E1A] = u'\u1E1B';
		table[0x1E1C] = u'\u1E1D';
		table[0x1E1E] = u'\u1E1F';
		table[0x1E20] = u'\u1E21';
		table[0x1E22] = u'\u1E23';
		table[0x1E24] = u'\u1E25';
		table[0x1E26] = u'\u1E27';
		table[0x1E28] = u'\u1E29';
		table[0x1E2A] = u'\u1E2B';
		table[0x1E2C] = u'\u1E2D';
		table[0x1E2E] = u'\u1E2F';
		table[0x1E30] = u'\u1E31';
		table[0x1E32] = u'\u1E33';
		table[0x1E34] = u'\u1E35';
		table[0x1E36] = u'\u1E37';
		table[0x1E38] = u'\u1E39';
		table[0x1E3A] = u'\u1E3B';
		table[0x1E3C] = u'\u1E3D';
		table[0x1E3E] = u'\u1E3F';
		table[0x1E40] = u'\u1E41';
		table[0x1E42] = u'\u1E43';
		table[0x1E44] = u'\u1E45';
		table[0x1E46] = u'\u1E47';
		table[0x1E48] = u'\u1E49';
		table[0x1E4A] = u'\u1E4B';
		table[0x1E4C] = u'\u1E4D';
		table[0x1E4E] = u'\u1E4F';
		table[0x1E50] = u'\u1E51';
		table[0x1E52] = u'\u1E53';
		table[0x1E54] = u'\u1E55';
		table[0x1E56] = u'\u1E57';
		table[0x1E58] = u'\u1E59';
		table[0x1E5A] = u'\u1E5B';
		table[0x1E5C] = u'\u1E5D';
		table[0x1E5E] = u'\u1E5F';
		table[0x1E60] = u'\u1E61';
		table[0x1E62] = u'\u1E63';
		table[0x1E64] = u'\u1E65';
		table[0x1E66] = u'\u1E67';
		table[0x1E68] = u'\u1E69';
		table[0x1E6A] = u'\u1E6B';
		table[0x1E6C] = u'\u1E6D';
		table[0x1E6E] = u'\u1E6F';
		table[0x1E70] = u'\u1E71';
		table[0x1E72] = u'\u1E73';
		table[0x1E74] = u'\u1E75';
		table[0x1E76] = u'\u1E77';
		table[0x1E78] = u'\u1E79';
		table[0x1E7A] = u'\u1E7B';
		table[0x1E7C] = u'\u1E7D';
		table[0x1E7E] = u'\u1E7F';
		table[0x1E80] = u'\u1E81';
		table[0x1E82] = u'\u1E83';
		table[0x1E84] = u'\u1E85';
		table[0x1E86] = u'\u1E87';
		table[0x1E88] = u'\u1E89';
		table[0x1E8A] = u'\u1E8B';
		table[0x1E8C] = u'\u1E8D';
		table[0x1E8E] = u'\u1E8F';
		table[0x1E90] = u'\u1E91';
		table[0x1E92] = u'\u1E93';
		table[0x1E94] = u'\u1E95';
		table[0x1E9B] = u'\u1E61';
		table[0x1E9E] = u'\u00DF';
		table[0x1EA0] = u'\u1EA1';
		table[0x1EA2] = u'\u1EA3';
		table[0x1EA4] = u'\u1EA5';
		table[0x1EA6] = u'\u1EA7';
		table[0x1EA8] = u'\u1EA9';
		table[0x1EAA] = u'\u1EAB';
		table[0x1EAC] = u'\u1EAD';
		table[0x1EAE] = u'\u1EAF';
		table[0x1EB0] = u'\u1EB1';
		table[0x1EB2] = u'\u1EB3';
		table[0x1EB4] = u'\u1EB5';
		table[0x1EB6] = u'\u1EB7';
		table[0x1EB8] = u'\u1EB9';
		table[0x1EBA] = u'\u1EBB';
		table[0x1EBC] = u'\u1EBD';
		table[0x1EBE] = u'\u1EBF';
		table[0x1EC0] = u'\u1EC1';
		table[0x1EC2] = u'\u1EC3';
		table[0x1EC4] = u'\u1EC5';
		table[0x1EC6] = u'\u1EC7';
		table[0x1EC8] = u'\u1EC9';
		table[0x1ECA] = u'\u1ECB';
		table[0x1ECC] = u'\u1ECD';
		table[0x1ECE] = u'\u1ECF';
		table[0x1ED0] = u'\u1ED1';
		table[0x1ED2] = u'\u1ED3';
		table[0x1ED4] = u'\u1ED5';
		table[0x1ED6] = u'\u1ED7';
		table[0x1ED8] = u'\u1ED9';
		table[0x1EDA] = u'\u1EDB';
		table[0x1EDC] = u'\u1EDD';
		table[0x1EDE] = u'\u1EDF';
		table[0x1EE0] = u'\u1EE1';
		table[0x1EE2] = u'\u1EE3';
		table[0x1EE4] = u'\u1EE5';
		table[0x1EE6] = u'\u1EE7';
		table[0x1EE8] = u'\u1EE9';
		table[0x1EEA] = u'\u1EEB';
		table[0x1EEC] = u'\u1EED';
		table[0x1EEE] = u'\u1EEF';
		table[0x1EF0] = u'\u1EF1';
		table[0x1EF2] = u'\u1EF3';
		table[0x1EF4] = u'\u1EF5';
		table[0x1EF6] = u'\u1EF7';
		table[0x1EF8] = u'\u1EF9';
		table[0x1EFA] = u'\u1EFB';
		table[0x1EFC] = u'\u1EFD';
		table[0x1EFE] = u'\u1EFF';
		table[0x1F08] = u'\u1F00';
		table[0x1F09] = u'\u1F01';
		table[0x1F0A] = u'\u1F02';
		table[0x1F0B] = u'\u1F03';
		table[0x1F0C] = u'\u1F04';
		table[0x1F0D] = u'\u1F05';
		table[0x1F0E] = u'\u1F06';
		table[0x1F0F] = u'\u1F07';
		table[0x1F18] = u'\u1F10';
		table[0x1F19] = u'\u1F11';
		table[0x1F1A] = u'\u1F12';
		table[0x1F1B] = u'\u1F13';
		table[0x1F1C] = u'\u1F14';
		table[0x1F1D] = u'\u1F15';
		table[0x1F28] = u'\u1F20';
		table[0x1F29] = u'\u1F21';
		table[0x1F2A] = u'\u1F22';
		table[0x1F2B] = u'\u1F23';
		table[0x1F2C] = u'\u1F24';
		table[0x1F2D] = u'\u1F25';
		table[0x1F2E] = u'\u1F26';
		table[0x1F2F] = u'\u1F27';
		table[0x1F38] = u'\u1F30';
		table[0x1F39] = u'\u1F31';
		table[0x1F3A] = u'\u1F32';
		table[0x1F3B] = u'\u1F33';
		table[0x1F3C] = u'\u1F34';
		table[0x1F3D] = u'\u1F35';
		table[0x1F3E] = u'\u1F36';
		table[0x1F3F] = u'\u1F37';
		table[0x1F48] = u'\u1F40';
		table[0x1F49] = u'\u1F41';
		table[0x1F4A] = u'\u1F42';
		table[0x1F4B] = u'\u1F43';
		table[0x1F4C] = u'\u1F44';
		table[0x1F4D] = u'\u1F45';
		table[0x1F59] = u'\u1F51';
		table[0x1F5B] = u'\u1F53';
		table[0x1F5D] = u'\u1F55';
		table[0x1F5F] = u'\u1F57';
		table[0x1F68] = u'\u1F60';
		table[0x1F69] = u'\u1F61';
		table[0x1F6A] = u'\u1F62';
		table[0x1F6B] = u'\u1F63';
		table[0x1F6C] = u'\u1F64';
		table[0x1F6D] = u'\u1F65';
		table[0x1F6E] = u'\u1F66';
		table[0x1F6F] = u'\u1F67';
		table[0x1F88] = u'\u1F80';
		table[0x1F89] = u'\u1F81';
		table[0x1F8A] = u'\u1F82';
		table[0x1F8B] = u'\u1F83';
		table[0x1F8C] = u'\u1F84';
		table[0x1F8D] = u'\u1F85';
		table[0x1F8E] = u'\u1F86';
		table[0x1F8F] = u'\u1F87';
		table[0x1F98] = u'\u1F90';
		table[0x1F99] = u'\u1F91';
		table[0x1F9A] = u'\u1F92';
		table[0x1F9B] = u'\u1F93';
		table[0x1F9C] = u'\u1F94';
		table[0x1F9D] = u'\u1F95';
		table[0x1F9E] = u'\u1F96';
		table[0x1F9F] = u'\u1F97';
		table[0x1FA8] = u'\u1FA0';
		table[0x1FA9] = u'\u1FA1';
		table[0x1FAA] = u'\u1FA2';
		table[0x1FAB] = u'\u1FA3';
		table[0x1FAC] = u'\u1FA4';
		table[0x1FAD] = u'\u1FA5';
		table[0x1FAE] = u'\u1FA6';
		table[0x1FAF] = u'\u1FA7';
		table[0x1FB8] = u'\u1FB0';
		table[0x1FB9] = u'\u1FB1';
		table[0x1FBA] = u'\u1F70';
		table[0x1FBB] = u'\u1F71';
		table[0x1FBC] = u'\u1FB3';
		table[0x1FBE] = u'\u03B9';
		table[0x1FC8] = u'\u1F72';
		table[0x1FC9] = u'\u1F73';
		table[0x1FCA] = u'\u1F74';
		table[0x1FCB] = u'\u1F75';
		table[0x1FCC] = u'\u1FC3';
		table[0x1FD3] = u'\u0390';
		table[0x1FD8] = u'\u1FD0';
		table[0x1FD9] = u'\u1FD1';
		table[0x1FDA] = u'\u1F76';
		table[0x1FDB] = u'\u1F77';
		table[0x1FE3] = u'\u03B0';
		table[0x1FE8] = u'\u1FE0';
		table[0x1FE9] = u'\u1FE1';
		table[0x1FEA] = u'\u1F7A';
		table[0x1FEB] = u'\u1F7B';
		table[0x1FEC] = u'\u1FE5';
		table[0x1FF8] = u'\u1F78';
		table[0x1FF9] = u'\u1F79';
		table[0x1FFA] = u'\u1F7C';
		table[0x1FFB] = u'\u1F7D';
		table[0x1FFC] = u'\u1FF3';
		table[0x2126] = u'\u03C9';
		table[0x212A] = u'\u006B';
		table[0x212B] = u'\u00E5';
		table[0x2132] = u'\u214E';
		table[0x2160] = u'\u2170';
		table[0x2161] = u'\u2171';
		table[0x2162] = u'\u2172';
		table[0x2163] = u'\u2173';
		table[0x2164] = u'\u2174';
		table[0x2165] = u'\u2175';
		table[0x2166] = u'\u2176';
		table[0x2167] = u'\u2177';
		table[0x2168] = u'\u2178';
		table[0x2169] = u'\u2179';
		table[0x216A] = u'\u217A';
		table[0x216B] = u'\u217B';
		table[0x216C] = u'\u217C';
		table[0x216D] = u'\u217D';
		table[0x216E] = u'\u217E';
		table[0x216F] = u'\u217F';
		table[0x2183] = u'\u2184';
		table[0x24B6] = u'\u24D0';
		table[0x24B7] = u'\u24D1';
		table[0x24B8] = u'\u24D2';
		table[0x24B9] = u'\u24D3';
		table[0x24BA] = u'\u24D4';
		table[0x24BB] = u'\u24D5';
		table[0x24BC] = u'\u24D6';
		table[0x24BD] = u'\u24D7';
		table[0x24BE] = u'\u24D8';
		table[0x24BF] = u'\u24D9';
		table[0x24C0] = u'\u24DA';
		table[0x24C1] = u'\u24DB';
		table[0x24C2] = u'\u24DC';
		table[0x24C3] = u'\u24DD';
		table[0x24C4] = u'\u24DE';
		table[0x24C5] = u'\u24DF';
		table[0x24C6] = u'\u24E0';
		table[0x24C7] = u'\u24E1';
		table[0x24C8] = u'\u24E2';
		table[0x24C9] = u'\u24E3';
		table[0x24CA] = u'\u24E4';
		table[0x24CB] = u'\u24E5';
		table[0x24CC] = u'\u24E6';
		table[0x24CD] = u'\u24E7';
		table[0x24CE] = u'\u24E8';
		table[0x24CF] = u'\u24E9';
		table[0x2C00] = u'\u2C30';
		table[0x2C01] = u'\u2C31';
		table[0x2C02] = u'\u2C32';
		table[0x2C03] = u'\u2C33';
		table[0x2C04] = u'\u2C34';
		table[0x2C05] = u'\u2C35';
		table[0x2C06] = u'\u2C36';
		table[0x2C07] = u'\u2C37';
		table[0x2C08] = u'\u2C38';
		table[0x2C09] = u'\u2C39';
		table[0x2C0A] = u'\u2C3A';
		table[0x2C0B] = u'\u2C3B';
		table[0x2C0C] = u'\u2C3C';
		table[0x2C0D] = u'\u2C3D';
		table[0x2C0E] = u'\u2C3E';
		table[0x2C0F] = u'\u2C3F';
		table[0x2C10] = u'\u2C40';
		table[0x2C11] = u'\u2C41';
		table[0x2C12] = u'\u2C42';
		table[0x2C13] = u'\u2C43';
		table[0x2C14] = u'\u2C44';
		table[0x2C15] = u'\u2C45';
		table[0x2C16] = u'\u2C46';
		table[0x2C17] = u'\u2C47';
		table[0x2C18] = u'\u2C48';
		table[0x2C19] = u'\u2C49';
		table[0x2C1A] = u'\u2C4A';
		table[0x2C1B] = u'\u2C4B';
		table[0x2C1C] = u'\u2C4C';
		table[0x2C1D] = u'\u2C4D';
		table[0x2C1E] = u'\u2C4E';
		table[0x2C1F] = u'\u2C4F';
		table[0x2C20] = u'\u2C50';
		table[0x2C21] = u'\u2C51';
		table[0x2C22] = u'\u2C52';
		table[0x2C23] = u'\u2C53';
		table[0x2C24] = u'\u2C54';
		table[0x2C25] = u'\u2C55';
		table[0x2C26] = u'\u2C56';
		table[0x2C27] = u'\u2C57';
		table[0x2C28] = u'\u2C58';
		table[0x2C29] = u'\u2C59';
		table[0x2C2A] = u'\u2C5A';
		table[0x2C2B] = u'\u2C5B';
		table[0x2C2C] = u'\u2C5C';
		table[0x2C2D] = u'\u2C5D';
		table[0x2C2E] = u'\u2C5E';
		table[0x2C2F] = u'\u2C5F';
		table[0x2C60] = u'\u2C61';
		table[0x2C62] = u'\u026B';
		table[0x2C63] = u'\u1D7D';
		table[0x2C64] = u'\u027D';
		table[0x2C67] = u'\u2C68';
		table[0x2C69] = u'\u2C6A';
		table[0x2C6B] = u'\u2C6C';
		table[0x2C6D] = u'\u0251';
		table[0x2C6E] = u'\u0271';
		table[0x2C6F] = u'\u0250';
		table[0x2C70] = u'\u0252';
		table[0x2C72] = u'\u2C73';
		table[0x2C75] = u'\u2C76';
		table[0x2C7E] = u'\u023F';
		table[0x2C7F] = u'\u0240';
		table[0x2C80] = u'\u2C81';
		table[0x2C82] = u'\u2C83';
		table[0x2C84] = u'\u2C85';
		table[0x2C86] = u'\u2C87';
		table[0x2C88] = u'\u2C89';
		table[0x2C8A] = u'\u2C8B';
		table[0x2C8C] = u'\u2C8D';
		table[0x2C8E] = u'\u2C8F';
		table[0x2C90] = u'\u2C91';
		table[0x2C92] = u'\u2C93';
		table[0x2C94] = u'\u2C95';
		table[0x2C96] = u'\u2C97';
		table[0x2C98] = u'\u2C99';
		table[0x2C9A] = u'\u2C9B';
		table[0x2C9C] = u'\u2C9D';
		table[0x2C9E] = u'\u2C9F';
		table[0x2CA0] = u'\u2CA1';
		table[0x2CA2] = u'\u2CA3';
		table[0x2CA4] = u'\u2CA5';
		table[0x2CA6] = u'\u2CA7';
		table[0x2CA8] = u'\u2CA9';
		table[0x2CAA] = u'\u2CAB';
		table[0x2CAC] = u'\u2CAD';
		table[0x2CAE] = u'\u2CAF';
		table[0x2CB0] = u'\u2CB1';
		table[0x2CB2] = u'\u2CB3';
		table[0x2CB4] = u'\u2CB5';
		table[0x2CB6] = u'\u2CB7';
		table[0x2CB8] = u'\u2CB9';
		table[0x2CBA] = u'\u2CBB';
		table[0x2CBC] = u'\u2CBD';
		table[0x2CBE] = u'\u2CBF';
		table[0x2CC0] = u'\u2CC1';
		table[0x2CC2] = u'\u2CC3';
		table[0x2CC4] = u'\u2CC5';
		table[0x2CC6] = u'\u2CC7';
		table[0x2CC8] = u'\u2CC9';
		table[0x2CCA] = u'\u2CCB';
		table[0x2CCC] = u'\u2CCD';
		table[0x2CCE] = u'\u2CCF';
		table[0x2CD0] = u'\u2CD1';
		table[0x2CD2] = u'\u2CD3';
		table[0x2CD4] = u'\u2CD5';
		table[0x2CD6] = u'\u2CD7';
		table[0x2CD8] = u'\u2CD9';
		table[0x2CDA] = u'\u2CDB';
		table[0x2CDC] = u'\u2CDD';
		table[0x2CDE] = u'\u2CDF';
		table[0x2CE0] = u'\u2CE1';
		table[0x2CE2] = u'\u2CE3';
		table[0x2CEB] = u'\u2CEC';
		table[0x2CED] = u'\u2CEE';
		table[0x2CF2] = u'\u2CF3';
		table[0xA640] = u'\uA641';
		table[0xA642] = u'\uA643';
		table[0xA644] = u'\uA645';
		table[0xA646] = u'\uA647';
		table[0xA648] = u'\uA649';
		table[0xA64A] = u'\uA64B';
		table[0xA64C] = u'\uA64D';
		table[0xA64E] = u'\uA64F';
		table[0xA650] = u'\uA651';
		table[0xA652] = u'\uA653';
		table[0xA654] = u'\uA655';
		table[0xA656] = u'\uA657';
		table[0xA658] = u'\uA659';
		table[0xA65A] = u'\uA65B';
		table[0xA65C] = u'\uA65D';
		table[0xA65E] = u'\uA65F';
		table[0xA660] = u'\uA661';
		table[0xA662] = u'\uA663';
		table[0xA664] = u'\uA665';
		table[0xA666] = u'\uA667';
		table[0xA668] = u'\uA669';
		table[0xA66A] = u'\uA66B';
		table[0xA66C] = u'\uA66D';
		table[0xA680] = u'\uA681';
		table[0xA682] = u'\uA683';
		table[0xA684] = u'\uA685';
		table[0xA686] = u'\uA687';
		table[0xA688] = u'\uA689';
		table[0xA68A] = u'\uA68B';
		table[0xA68C] = u'\uA68D';
		table[0xA68E] = u'\uA68F';
		table[0xA690] = u'\uA691';
		table[0xA692] = u'\uA693';
		table[0xA694] = u'\uA695';
		table[0xA696] = u'\uA697';
		table[0xA698] = u'\uA699';
		table[0xA69A] = u'\uA69B';
		table[0xA722] = u'\uA723';
		table[0xA724] = u'\uA725';
		table[0xA726] = u'\uA727';
		table[0xA728] = u'\uA729';
		table[0xA72A] = u'\uA72B';
		table[0xA72C] = u'\uA72D';
		table[0xA72E] = u'\uA72F';
		table[0xA732] = u'\uA733';
		table[0xA734] = u'\uA735';
		table[0xA736] = u'\uA737';
		table[0xA738] = u'\uA739';
		table[0xA73A] = u'\uA73B';
		table[0xA73C] = u'\uA73D';
		table[0xA73E] = u'\uA73F';
		table[0xA740] = u'\uA741';
		table[0xA742] = u'\uA743';
		table[0xA744] = u'\uA745';
		table[0xA746] = u'\uA747';
		table[0xA748] = u'\uA749';
		table[0xA74A] = u'\uA74B';
		table[0xA74C] = u'\uA74D';
		table[0xA74E] = u'\uA74F';
		table[0xA750] = u'\uA751';
		table[0xA752] = u'\uA753';
		table[0xA754] = u'\uA755';
		table[0xA756] = u'\uA757';
		table[0xA758] = u'\uA759';
		table[0xA75A] = u'\uA75B';
		table[0xA75C] = u'\uA75D';
		table[0xA75E] = u'\uA75F';
		table[0xA760] = u'\uA761';
		table[0xA762] = u'\uA763';
		table[0xA764] = u'\uA765';
		table[0xA766] = u'\uA767';
		table[0xA768] = u'\uA769';
		table[0xA76A] = u'\uA76B';
		table[0xA76C] = u'\uA76D';
		table[0xA76E] = u'\uA76F';
		table[0xA779] = u'\uA77A';
		table[0xA77B] = u'\uA77C';
		table[0xA77D] = u'\u1D79';
		table[0xA77E] = u'\uA77F';
		table[0xA780] = u'\uA781';
		table[0xA782] = u'\uA783';
		table[0xA784] = u'\uA785';
		table[0xA786] = u'\uA787';
		table[0xA78B] = u'\uA78C';
		table[0xA78D] = u'\u0265';
		table[0xA790] = u'\uA791';
		table[0xA792] = u'\uA793';
		table[0xA796] = u'\uA797';
		table[0xA798] = u'\uA799';
		table[0xA79A] = u'\uA79B';
		table[0xA79C] = u'\uA79D';
		table[0xA79E] = u'\uA79F';
		table[0xA7A0] = u'\uA7A1';
		table[0xA7A2] = u'\uA7A3';
		table[0xA7A4] = u'\uA7A5';
		table[0xA7A6] = u'\uA7A7';
		table[0xA7A8] = u'\uA7A9';
		table[0xA7AA] = u'\u0266';
		table[0xA7AB] = u'\u025C';
		table[0xA7AC] = u'\u0261';
		table[0xA7AD] = u'\u026C';
		table[0xA7AE] = u'\u026A';
		table[0xA7B0] = u'\u029E';
		table[0xA7B1] = u'\u0287';
		table[0xA7B2] = u'\u029D';
		table[0xA7B3] = u'\uAB53';
		table[0xA7B4] = u'\uA7B5';
		table[0xA7B6] = u'\uA7B7';
		table[0xA7B8] = u'\uA7B9';
		table[0xA7BA] = u'\uA7BB';
		table[0xA7BC] = u'\uA7BD';
		table[0xA7BE] = u'\uA7BF';
		table[0xA7C0] = u'\uA7C1';
		table[0xA7C2] = u'\uA7C3';
		table[0xA7C4] = u'\uA794';
		table[0xA7C5] = u'\u0282';
		table[0xA7C6] = u'\u1D8E';
		table[0xA7C7] = u'\uA7C8';
		table[0xA7C9] = u'\uA7CA';
		table[0xA7D0] = u'\uA7D1';
		table[0xA7D6] = u'\uA7D7';
		table[0xA7D8] = u'\uA7D9';
		table[0xA7F5] = u'\uA7F6';
		table[0xAB70] = u'\u13A0';
		table[0xAB71] = u'\u13A1';
		table[0xAB72] = u'\u13A2';
		table[0xAB73] = u'\u13A3';
		table[0xAB74] = u'\u13A4';
		table[0xAB75] = u'\u13A5';
		table[0xAB76] = u'\u13A6';
		table[0xAB77] = u'\u13A7';
		table[0xAB78] = u'\u13A8';
		table[0xAB79] = u'\u13A9';
		table[0xAB7A] = u'\u13AA';
		table[0xAB7B] = u'\u13AB';
		table[0xAB7C] = u'\u13AC';
		table[0xAB7D] = u'\u13AD';
		table[0xAB7E] = u'\u13AE';
		table[0xAB7F] = u'\u13AF';
		table[0xAB80] = u'\u13B0';
		table[0xAB81] = u'\u13B1';
		table[0xAB82] = u'\u13B2';
		table[0xAB83] = u'\u13B3';
		table[0xAB84] = u'\u13B4';
		table[0xAB85] = u'\u13B5';
		table[0xAB86] = u'\u13B6';
		table[0xAB87] = u'\u13B7';
		table[0xAB88] = u'\u13B8';
		table[0xAB89] = u'\u13B9';
		table[0xAB8A] = u'\u13BA';
		table[0xAB8B] = u'\u13BB';
		table[0xAB8C] = u'\u13BC';
		table[0xAB8D] = u'\u13BD';
		table[0xAB8E] = u'\u13BE';
		table[0xAB8F] = u'\u13BF';
		table[0xAB90] = u'\u13C0';
		table[0xAB91] = u'\u13C1';
		table[0xAB92] = u'\u13C2';
		table[0xAB93] = u'\u13C3';
		table[0xAB94] = u'\u13C4';
		table[0xAB95] = u'\u13C5';
		table[0xAB96] = u'\u13C6';
		table[0xAB97] = u'\u13C7';
		table[0xAB98] = u'\u13C8';
		table[0xAB99] = u'\u13C9';
		table[0xAB9A] = u'\u13CA';
		table[0xAB9B] = u'\u13CB';
		table[0xAB9C] = u'\u13CC';
		table[0xAB9D] = u'\u13CD';
		table[0xAB9E] = u'\u13CE';
		table[0xAB9F] = u'\u13CF';
		table[0xABA0] = u'\u13D0';
		table[0xABA1] = u'\u13D1';
		table[0xABA2] = u'\u13D2';
		table[0xABA3] = u'\u13D3';
		table[0xABA4] = u'\u13D4';
		table[0xABA5] = u'\u13D5';
		table[0xABA6] = u'\u13D6';
		table[0xABA7] = u'\u13D7';
		table[0xABA8] = u'\u13D8';
		table[0xABA9] = u'\u13D9';
		table[0xABAA] = u'\u13DA';
		table[0xABAB] = u'\u13DB';
		table[0xABAC] = u'\u13DC';
		table[0xABAD] = u'\u13DD';
		table[0xABAE] = u'\u13DE';
		table[0xABAF] = u'\u13DF';
		table[0xABB0] = u'\u13E0';
		table[0xABB1] = u'\u13E1';
		table[0xABB2] = u'\u13E2';
		table[0xABB3] = u'\u13E3';
		table[0xABB4] = u'\u13E4';
		table[0xABB5] = u'\u13E5';
		table[0xABB6] = u'\u13E6';
		table[0xABB7] = u'\u13E7';
		table[0xABB8] = u'\u13E8';
		table[0xABB9] = u'\u13E9';
		table[0xABBA] = u'\u13EA';
		table[0xABBB] = u'\u13EB';
		table[0xABBC] = u'\u13EC';
		table[0xABBD] = u'\u13ED';
		table[0xABBE] = u'\u13EE';
		table[0xABBF] = u'\u13EF';
		table[0xFB05] = u'\uFB06';
		table[0xFF21] = u'\uFF41';
		table[0xFF22] = u'\uFF42';
		table[0xFF23] = u'\uFF43';
		table[0xFF24] = u'\uFF44';
		table[0xFF25] = u'\uFF45';
		table[0xFF26] = u'\uFF46';
		table[0xFF27] = u'\uFF47';
		table[0xFF28] = u'\uFF48';
		table[0xFF29] = u'\uFF49';
		table[0xFF2A] = u'\uFF4A';
		table[0xFF2B] = u'\uFF4B';
		table[0xFF2C] = u'\uFF4C';
		table[0xFF2D] = u'\uFF4D';
		table[0xFF2E] = u'\uFF4E';
		table[0xFF2F] = u'\uFF4F';
		table[0xFF30] = u'\uFF50';
		table[0xFF31] = u'\uFF51';
		table[0xFF32] = u'\uFF52';
		table[0xFF33] = u'\uFF53';
		table[0xFF34] = u'\uFF54';
		table[0xFF35] = u'\uFF55';
		table[0xFF36] = u'\uFF56';
		table[0xFF37] = u'\uFF57';
		table[0xFF38] = u'\uFF58';
		table[0xFF39] = u'\uFF59';
		table[0xFF3A] = u'\uFF5A';

		return table;
	}();

	// https://www.unicode.org/Public/16.0.0/ucd/CaseFolding.txt
	constexpr auto CaseFoldingTable1 = [] static {

		auto table = std::array<char16_t, 0x10000>{};
		std::ranges::iota(table, 0);

		table[0x0400] = u'\u0428';
		table[0x0401] = u'\u0429';
		table[0x0402] = u'\u042A';
		table[0x0403] = u'\u042B';
		table[0x0404] = u'\u042C';
		table[0x0405] = u'\u042D';
		table[0x0406] = u'\u042E';
		table[0x0407] = u'\u042F';
		table[0x0408] = u'\u0430';
		table[0x0409] = u'\u0431';
		table[0x040A] = u'\u0432';
		table[0x040B] = u'\u0433';
		table[0x040C] = u'\u0434';
		table[0x040D] = u'\u0435';
		table[0x040E] = u'\u0436';
		table[0x040F] = u'\u0437';
		table[0x0410] = u'\u0438';
		table[0x0411] = u'\u0439';
		table[0x0412] = u'\u043A';
		table[0x0413] = u'\u043B';
		table[0x0414] = u'\u043C';
		table[0x0415] = u'\u043D';
		table[0x0416] = u'\u043E';
		table[0x0417] = u'\u043F';
		table[0x0418] = u'\u0440';
		table[0x0419] = u'\u0441';
		table[0x041A] = u'\u0442';
		table[0x041B] = u'\u0443';
		table[0x041C] = u'\u0444';
		table[0x041D] = u'\u0445';
		table[0x041E] = u'\u0446';
		table[0x041F] = u'\u0447';
		table[0x0420] = u'\u0448';
		table[0x0421] = u'\u0449';
		table[0x0422] = u'\u044A';
		table[0x0423] = u'\u044B';
		table[0x0424] = u'\u044C';
		table[0x0425] = u'\u044D';
		table[0x0426] = u'\u044E';
		table[0x0427] = u'\u044F';
		table[0x04B0] = u'\u04D8';
		table[0x04B1] = u'\u04D9';
		table[0x04B2] = u'\u04DA';
		table[0x04B3] = u'\u04DB';
		table[0x04B4] = u'\u04DC';
		table[0x04B5] = u'\u04DD';
		table[0x04B6] = u'\u04DE';
		table[0x04B7] = u'\u04DF';
		table[0x04B8] = u'\u04E0';
		table[0x04B9] = u'\u04E1';
		table[0x04BA] = u'\u04E2';
		table[0x04BB] = u'\u04E3';
		table[0x04BC] = u'\u04E4';
		table[0x04BD] = u'\u04E5';
		table[0x04BE] = u'\u04E6';
		table[0x04BF] = u'\u04E7';
		table[0x04C0] = u'\u04E8';
		table[0x04C1] = u'\u04E9';
		table[0x04C2] = u'\u04EA';
		table[0x04C3] = u'\u04EB';
		table[0x04C4] = u'\u04EC';
		table[0x04C5] = u'\u04ED';
		table[0x04C6] = u'\u04EE';
		table[0x04C7] = u'\u04EF';
		table[0x04C8] = u'\u04F0';
		table[0x04C9] = u'\u04F1';
		table[0x04CA] = u'\u04F2';
		table[0x04CB] = u'\u04F3';
		table[0x04CC] = u'\u04F4';
		table[0x04CD] = u'\u04F5';
		table[0x04CE] = u'\u04F6';
		table[0x04CF] = u'\u04F7';
		table[0x04D0] = u'\u04F8';
		table[0x04D1] = u'\u04F9';
		table[0x04D2] = u'\u04FA';
		table[0x04D3] = u'\u04FB';
		table[0x0570] = u'\u0597';
		table[0x0571] = u'\u0598';
		table[0x0572] = u'\u0599';
		table[0x0573] = u'\u059A';
		table[0x0574] = u'\u059B';
		table[0x0575] = u'\u059C';
		table[0x0576] = u'\u059D';
		table[0x0577] = u'\u059E';
		table[0x0578] = u'\u059F';
		table[0x0579] = u'\u05A0';
		table[0x057A] = u'\u05A1';
		table[0x057C] = u'\u05A3';
		table[0x057D] = u'\u05A4';
		table[0x057E] = u'\u05A5';
		table[0x057F] = u'\u05A6';
		table[0x0580] = u'\u05A7';
		table[0x0581] = u'\u05A8';
		table[0x0582] = u'\u05A9';
		table[0x0583] = u'\u05AA';
		table[0x0584] = u'\u05AB';
		table[0x0585] = u'\u05AC';
		table[0x0586] = u'\u05AD';
		table[0x0587] = u'\u05AE';
		table[0x0588] = u'\u05AF';
		table[0x0589] = u'\u05B0';
		table[0x058A] = u'\u05B1';
		table[0x058C] = u'\u05B3';
		table[0x058D] = u'\u05B4';
		table[0x058E] = u'\u05B5';
		table[0x058F] = u'\u05B6';
		table[0x0590] = u'\u05B7';
		table[0x0591] = u'\u05B8';
		table[0x0592] = u'\u05B9';
		table[0x0594] = u'\u05BB';
		table[0x0595] = u'\u05BC';
		table[0x0C80] = u'\u0CC0';
		table[0x0C81] = u'\u0CC1';
		table[0x0C82] = u'\u0CC2';
		table[0x0C83] = u'\u0CC3';
		table[0x0C84] = u'\u0CC4';
		table[0x0C85] = u'\u0CC5';
		table[0x0C86] = u'\u0CC6';
		table[0x0C87] = u'\u0CC7';
		table[0x0C88] = u'\u0CC8';
		table[0x0C89] = u'\u0CC9';
		table[0x0C8A] = u'\u0CCA';
		table[0x0C8B] = u'\u0CCB';
		table[0x0C8C] = u'\u0CCC';
		table[0x0C8D] = u'\u0CCD';
		table[0x0C8E] = u'\u0CCE';
		table[0x0C8F] = u'\u0CCF';
		table[0x0C90] = u'\u0CD0';
		table[0x0C91] = u'\u0CD1';
		table[0x0C92] = u'\u0CD2';
		table[0x0C93] = u'\u0CD3';
		table[0x0C94] = u'\u0CD4';
		table[0x0C95] = u'\u0CD5';
		table[0x0C96] = u'\u0CD6';
		table[0x0C97] = u'\u0CD7';
		table[0x0C98] = u'\u0CD8';
		table[0x0C99] = u'\u0CD9';
		table[0x0C9A] = u'\u0CDA';
		table[0x0C9B] = u'\u0CDB';
		table[0x0C9C] = u'\u0CDC';
		table[0x0C9D] = u'\u0CDD';
		table[0x0C9E] = u'\u0CDE';
		table[0x0C9F] = u'\u0CDF';
		table[0x0CA0] = u'\u0CE0';
		table[0x0CA1] = u'\u0CE1';
		table[0x0CA2] = u'\u0CE2';
		table[0x0CA3] = u'\u0CE3';
		table[0x0CA4] = u'\u0CE4';
		table[0x0CA5] = u'\u0CE5';
		table[0x0CA6] = u'\u0CE6';
		table[0x0CA7] = u'\u0CE7';
		table[0x0CA8] = u'\u0CE8';
		table[0x0CA9] = u'\u0CE9';
		table[0x0CAA] = u'\u0CEA';
		table[0x0CAB] = u'\u0CEB';
		table[0x0CAC] = u'\u0CEC';
		table[0x0CAD] = u'\u0CED';
		table[0x0CAE] = u'\u0CEE';
		table[0x0CAF] = u'\u0CEF';
		table[0x0CB0] = u'\u0CF0';
		table[0x0CB1] = u'\u0CF1';
		table[0x0CB2] = u'\u0CF2';
		table[0x18A0] = u'\u18C0';
		table[0x18A1] = u'\u18C1';
		table[0x18A2] = u'\u18C2';
		table[0x18A3] = u'\u18C3';
		table[0x18A4] = u'\u18C4';
		table[0x18A5] = u'\u18C5';
		table[0x18A6] = u'\u18C6';
		table[0x18A7] = u'\u18C7';
		table[0x18A8] = u'\u18C8';
		table[0x18A9] = u'\u18C9';
		table[0x18AA] = u'\u18CA';
		table[0x18AB] = u'\u18CB';
		table[0x18AC] = u'\u18CC';
		table[0x18AD] = u'\u18CD';
		table[0x18AE] = u'\u18CE';
		table[0x18AF] = u'\u18CF';
		table[0x18B0] = u'\u18D0';
		table[0x18B1] = u'\u18D1';
		table[0x18B2] = u'\u18D2';
		table[0x18B3] = u'\u18D3';
		table[0x18B4] = u'\u18D4';
		table[0x18B5] = u'\u18D5';
		table[0x18B6] = u'\u18D6';
		table[0x18B7] = u'\u18D7';
		table[0x18B8] = u'\u18D8';
		table[0x18B9] = u'\u18D9';
		table[0x18BA] = u'\u18DA';
		table[0x18BB] = u'\u18DB';
		table[0x18BC] = u'\u18DC';
		table[0x18BD] = u'\u18DD';
		table[0x18BE] = u'\u18DE';
		table[0x18BF] = u'\u18DF';
		table[0x6E40] = u'\u6E60';
		table[0x6E41] = u'\u6E61';
		table[0x6E42] = u'\u6E62';
		table[0x6E43] = u'\u6E63';
		table[0x6E44] = u'\u6E64';
		table[0x6E45] = u'\u6E65';
		table[0x6E46] = u'\u6E66';
		table[0x6E47] = u'\u6E67';
		table[0x6E48] = u'\u6E68';
		table[0x6E49] = u'\u6E69';
		table[0x6E4A] = u'\u6E6A';
		table[0x6E4B] = u'\u6E6B';
		table[0x6E4C] = u'\u6E6C';
		table[0x6E4D] = u'\u6E6D';
		table[0x6E4E] = u'\u6E6E';
		table[0x6E4F] = u'\u6E6F';
		table[0x6E50] = u'\u6E70';
		table[0x6E51] = u'\u6E71';
		table[0x6E52] = u'\u6E72';
		table[0x6E53] = u'\u6E73';
		table[0x6E54] = u'\u6E74';
		table[0x6E55] = u'\u6E75';
		table[0x6E56] = u'\u6E76';
		table[0x6E57] = u'\u6E77';
		table[0x6E58] = u'\u6E78';
		table[0x6E59] = u'\u6E79';
		table[0x6E5A] = u'\u6E7A';
		table[0x6E5B] = u'\u6E7B';
		table[0x6E5C] = u'\u6E7C';
		table[0x6E5D] = u'\u6E7D';
		table[0x6E5E] = u'\u6E7E';
		table[0x6E5F] = u'\u6E7F';
		table[0xE900] = u'\uE922';
		table[0xE901] = u'\uE923';
		table[0xE902] = u'\uE924';
		table[0xE903] = u'\uE925';
		table[0xE904] = u'\uE926';
		table[0xE905] = u'\uE927';
		table[0xE906] = u'\uE928';
		table[0xE907] = u'\uE929';
		table[0xE908] = u'\uE92A';
		table[0xE909] = u'\uE92B';
		table[0xE90A] = u'\uE92C';
		table[0xE90B] = u'\uE92D';
		table[0xE90C] = u'\uE92E';
		table[0xE90D] = u'\uE92F';
		table[0xE90E] = u'\uE930';
		table[0xE90F] = u'\uE931';
		table[0xE910] = u'\uE932';
		table[0xE911] = u'\uE933';
		table[0xE912] = u'\uE934';
		table[0xE913] = u'\uE935';
		table[0xE914] = u'\uE936';
		table[0xE915] = u'\uE937';
		table[0xE916] = u'\uE938';
		table[0xE917] = u'\uE939';
		table[0xE918] = u'\uE93A';
		table[0xE919] = u'\uE93B';
		table[0xE91A] = u'\uE93C';
		table[0xE91B] = u'\uE93D';
		table[0xE91C] = u'\uE93E';
		table[0xE91D] = u'\uE93F';
		table[0xE91E] = u'\uE940';
		table[0xE91F] = u'\uE941';
		table[0xE920] = u'\uE942';
		table[0xE921] = u'\uE943';

		return table;
	}();
}

namespace Citrine {

	auto Unicode::FoldCase(std::wstring_view input) -> std::wstring {

		auto output = std::wstring{};
		FoldCase(input, output);
		return output;
	}

	auto Unicode::FoldCase(std::wstring_view input, std::wstring& output) -> void {

		output.clear();
		output.resize_and_overwrite(input.size(), [&](wchar_t* const data, std::size_t size) {

			auto in = input.data();
			auto const end = in + input.size();
			auto out = data;

			while (in < end) {

				auto high = std::size_t{ *in++ };
				if ((high & SurrogateMask) == HighSurrogateValue && in < end && (*in & SurrogateMask) == LowSurrogateValue) {

					auto low = std::size_t{ *in++ };
					auto codepoint = (high & SurrogateCodepointMask) << SurrogateCodepointBits;
					codepoint |= (low & SurrogateCodepointMask);

					if (codepoint < CaseFoldingTable1.size()) {

						codepoint = CaseFoldingTable1[codepoint];
						high = HighSurrogateValue | (codepoint >> SurrogateCodepointBits);
						low = LowSurrogateValue | (codepoint & SurrogateCodepointMask);
					}

					*out++ = static_cast<wchar_t>(high);
					*out++ = static_cast<wchar_t>(low);
				}
				else {

					high = CaseFoldingTable0[high];
					*out++ = static_cast<wchar_t>(high);
				}
			}

			return size;
		});
	}

	auto Unicode::FoldCaseInPlace(std::wstring& str) noexcept -> void {

		auto it = str.data();
		auto const end = it + str.size();
		auto dest = str.data();

		while (it < end) {

			auto high = std::size_t{ *it++ };
			if ((high & SurrogateMask) == HighSurrogateValue && it < end && (*it & SurrogateMask) == LowSurrogateValue) {

				auto low = std::size_t{ *it++ };
				auto codepoint = (high & SurrogateCodepointMask) << SurrogateCodepointBits;
				codepoint |= (low & SurrogateCodepointMask);

				if (codepoint < CaseFoldingTable1.size()) {

					codepoint = CaseFoldingTable1[codepoint];
					high = HighSurrogateValue | (codepoint >> SurrogateCodepointBits);
					low = LowSurrogateValue | (codepoint & SurrogateCodepointMask);
				}

				*dest++ = static_cast<wchar_t>(high);
				*dest++ = static_cast<wchar_t>(low);
			}
			else {

				high = CaseFoldingTable0[high];
				*dest++ = static_cast<wchar_t>(high);
			}
		}
	}

	auto Unicode::IsWhitespace(wchar_t ch) noexcept -> bool {

		return CaseFoldingTable0[ch] == u'\u0020';
	}

	auto Unicode::FindFirstWhitespace(std::wstring_view input, std::size_t offset) noexcept -> std::size_t {

		if (offset < input.size()) {

			auto const begin = input.data();
			auto const end = begin + input.size();

			for (auto in = begin + offset; in < end; ++in) {

				if (CaseFoldingTable0[*in] == u'\u0020')
					return in - begin;
			}
		}

		return input.npos;
	}

	auto Unicode::FindFirstNonWhitespace(std::wstring_view input, std::size_t offset) noexcept -> std::size_t {

		if (offset < input.size()) {

			auto const begin = input.data();
			auto const end = begin + input.size();

			for (auto in = begin + offset; in < end; ++in) {

				if (CaseFoldingTable0[*in] != u'\u0020')
					return in - begin;
			}
		}

		return input.npos;
	}

	auto Unicode::FindLastWhitespace(std::wstring_view input, std::size_t offset) noexcept -> std::size_t {

		if (input.size() > 0) {

			auto const begin = input.data();
			for (auto in = begin + std::min(offset, input.size() - 1);; --in) {

				if (CaseFoldingTable0[*in] == u'\u0020')
					return in - begin;

				if (in == begin)
					break;
			}
		}

		return input.npos;
	}

	auto Unicode::FindLastNonWhitespace(std::wstring_view input, std::size_t offset) noexcept -> std::size_t {

		if (input.size() > 0) {

			auto const begin = input.data();
			for (auto in = begin + std::min(offset, input.size() - 1);; --in) {

				if (CaseFoldingTable0[*in] != u'\u0020')
					return in - begin;

				if (in == begin)
					break;
			}
		}

		return input.npos;
	}

	auto Unicode::NormalizeWhitespace(std::wstring_view input) -> std::wstring {

		auto output = std::wstring{};
		NormalizeWhitespace(input, output);
		return output;
	}

	auto Unicode::NormalizeWhitespace(std::wstring_view input, std::wstring& output) -> void {

		input.remove_suffix(input.size() - FindLastNonWhitespace(input) - 1);

		output.clear();
		output.resize_and_overwrite(input.size(), [&](wchar_t* const data, std::size_t) {

			auto in = input.data();
			auto const end = in + input.size();
			auto out = data;

			while (true) {

				while (in < end && CaseFoldingTable0[*in] == u'\u0020') ++in;
				while (in < end) {

					auto ch = CaseFoldingTable0[*in++];
					*out++ = static_cast<wchar_t>(ch);
					if (ch == u'\u0020') break;
				}
				if (in == end) break;
			}
			return out - data;
		});
	}

	auto Unicode::NormalizeWhitespaceInPlace(std::wstring& str) noexcept -> void {

		str.erase(FindLastNonWhitespace(str) + 1);

		auto it = str.data();
		auto const end = it + str.size();
		auto dest = str.data();

		while (true) {

			while (it < end && CaseFoldingTable0[*it] == u'\u0020') ++it;
			while (it < end) {

				auto ch = CaseFoldingTable0[*it++];
				*dest++ = static_cast<wchar_t>(ch);
				if (ch == u'\u0020') break;
			}
			if (it == end) break;
		}
		str.erase(dest - str.data());
	}
}