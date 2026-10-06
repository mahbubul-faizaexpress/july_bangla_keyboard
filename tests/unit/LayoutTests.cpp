#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>

#include "JulyTest.h"
#include "july/engine/Layout.h"

using july::KeyLayer;
using july::TokenKind;

namespace {

// Expectations are restated here by hand from the Bijoy 3rd-edition chart (the
// high-resolution image supplied 2026-10-06), with scan
// codes written independently of layoutgen's key-name table, so a mistake in either the
// layout file or the generator shows up as a mismatch.
struct Expect {
    std::uint16_t scan;
    KeyLayer layer;
    TokenKind kind;
    std::u16string_view text;
};

constexpr Expect kExpected[] = {
    // Top row: Q W E R T Y U I O P = 0x10..0x19
    {0x10, KeyLayer::Normal, TokenKind::Consonant, u"ঙ"},  // ঙ
    {0x10, KeyLayer::Shift, TokenKind::Modifier, u"ং"},    // ং
    {0x11, KeyLayer::Normal, TokenKind::Consonant, u"য"},  // য
    {0x11, KeyLayer::Shift, TokenKind::Consonant, u"য়"},   // য়
    {0x12, KeyLayer::Normal, TokenKind::Consonant, u"ড"},  // ড
    {0x12, KeyLayer::Shift, TokenKind::Consonant, u"ঢ"},   // ঢ
    {0x13, KeyLayer::Normal, TokenKind::Consonant, u"প"},  // প
    {0x13, KeyLayer::Shift, TokenKind::Consonant, u"ফ"},   // ফ
    {0x14, KeyLayer::Normal, TokenKind::Consonant, u"ট"},  // ট
    {0x14, KeyLayer::Shift, TokenKind::Consonant, u"ঠ"},   // ঠ
    {0x15, KeyLayer::Normal, TokenKind::Consonant, u"চ"},  // চ
    {0x15, KeyLayer::Shift, TokenKind::Consonant, u"ছ"},   // ছ
    {0x16, KeyLayer::Normal, TokenKind::Consonant, u"জ"},  // জ
    {0x16, KeyLayer::Shift, TokenKind::Consonant, u"ঝ"},   // ঝ
    {0x17, KeyLayer::Normal, TokenKind::Consonant, u"হ"},  // হ
    {0x17, KeyLayer::Shift, TokenKind::Consonant, u"ঞ"},   // ঞ
    {0x18, KeyLayer::Normal, TokenKind::Consonant, u"গ"},  // গ
    {0x18, KeyLayer::Shift, TokenKind::Consonant, u"ঘ"},   // ঘ
    {0x19, KeyLayer::Normal, TokenKind::Consonant, u"ড়"},  // ড়
    {0x19, KeyLayer::Shift, TokenKind::Consonant, u"ঢ়"},   // ঢ়
    // Home row: A S D F G H J K L = 0x1E..0x26, \ = 0x2B
    {0x1E, KeyLayer::Normal, TokenKind::VowelSign, u"ৃ"},        // ৃ
    {0x1E, KeyLayer::Shift, TokenKind::Reph, u"র্"},        // র্
    {0x1E, KeyLayer::Link, TokenKind::IndependentVowel, u"ঋ"},   // ঋ
    {0x1F, KeyLayer::Normal, TokenKind::VowelSign, u"ু"},        // ু
    {0x1F, KeyLayer::Shift, TokenKind::VowelSign, u"ূ"},         // ূ
    {0x1F, KeyLayer::Link, TokenKind::IndependentVowel, u"উ"},   // উ
    {0x1F, KeyLayer::LinkShift, TokenKind::IndependentVowel, u"ঊ"},  // ঊ
    {0x20, KeyLayer::Normal, TokenKind::VowelSignPre, u"ি"},     // ি
    {0x20, KeyLayer::Shift, TokenKind::VowelSign, u"ী"},         // ী
    {0x20, KeyLayer::Link, TokenKind::IndependentVowel, u"ই"},   // ই
    {0x20, KeyLayer::LinkShift, TokenKind::IndependentVowel, u"ঈ"},  // ঈ
    {0x21, KeyLayer::Normal, TokenKind::VowelSign, u"া"},        // া
    {0x21, KeyLayer::Shift, TokenKind::IndependentVowel, u"অ"},  // অ
    {0x21, KeyLayer::Link, TokenKind::IndependentVowel, u"আ"},   // আ
    {0x22, KeyLayer::Normal, TokenKind::Link, u"্"},             // ্
    {0x22, KeyLayer::Shift, TokenKind::Punct, u"।"},             // ।
    {0x23, KeyLayer::Normal, TokenKind::Consonant, u"ব"},        // ব
    {0x23, KeyLayer::Shift, TokenKind::Consonant, u"ভ"},         // ভ
    {0x24, KeyLayer::Normal, TokenKind::Consonant, u"ক"},        // ক
    {0x24, KeyLayer::Shift, TokenKind::Consonant, u"খ"},         // খ
    {0x25, KeyLayer::Normal, TokenKind::Consonant, u"ত"},        // ত
    {0x25, KeyLayer::Shift, TokenKind::Consonant, u"থ"},         // থ
    {0x26, KeyLayer::Normal, TokenKind::Consonant, u"দ"},        // দ
    {0x26, KeyLayer::Shift, TokenKind::Consonant, u"ধ"},         // ধ
    {0x2B, KeyLayer::Normal, TokenKind::Final, u"ৎ"},            // ৎ
    {0x2B, KeyLayer::Shift, TokenKind::Modifier, u"ঃ"},          // ঃ
    // Bottom row: Z X C V B N M = 0x2C..0x32
    {0x2C, KeyLayer::Normal, TokenKind::Phala, u"্র"},      // ্র
    {0x2C, KeyLayer::Shift, TokenKind::Phala, u"্য"},       // ্য
    {0x2D, KeyLayer::Normal, TokenKind::IndependentVowel, u"ও"}, // ও
    {0x2D, KeyLayer::Shift, TokenKind::VowelSign, u"ৗ"},         // ৗ
    {0x2D, KeyLayer::LinkShift, TokenKind::IndependentVowel, u"ঔ"},  // ঔ (G + Shift+X)
    {0x2E, KeyLayer::Normal, TokenKind::VowelSignPre, u"ে"},     // ে
    {0x2E, KeyLayer::Shift, TokenKind::VowelSignPre, u"ৈ"},      // ৈ
    {0x2E, KeyLayer::Link, TokenKind::IndependentVowel, u"এ"},   // এ
    {0x2E, KeyLayer::LinkShift, TokenKind::IndependentVowel, u"ঐ"},  // ঐ
    {0x2F, KeyLayer::Normal, TokenKind::Consonant, u"র"},        // র
    {0x2F, KeyLayer::Shift, TokenKind::Consonant, u"ল"},         // ল
    {0x30, KeyLayer::Normal, TokenKind::Consonant, u"ন"},        // ন
    {0x30, KeyLayer::Shift, TokenKind::Consonant, u"ণ"},         // ণ
    {0x31, KeyLayer::Normal, TokenKind::Consonant, u"স"},        // স
    {0x31, KeyLayer::Shift, TokenKind::Consonant, u"ষ"},         // ষ
    {0x32, KeyLayer::Normal, TokenKind::Consonant, u"ম"},        // ম
    {0x32, KeyLayer::Shift, TokenKind::Consonant, u"শ"},         // শ
    // Shifted number row: 4 = 0x05, 7 = 0x08
    {0x05, KeyLayer::Shift, TokenKind::Punct, u"৳"},             // ৳
    {0x08, KeyLayer::Shift, TokenKind::Modifier, u"ঁ"},          // ঁ
    // Quote keys: ` = 0x29, ' = 0x28
    {0x29, KeyLayer::Normal, TokenKind::Punct, u"‘"},       // ‘
    {0x29, KeyLayer::Shift, TokenKind::Punct, u"“"},        // “
    {0x28, KeyLayer::Normal, TokenKind::Punct, u"’"},       // ’
    {0x28, KeyLayer::Shift, TokenKind::Punct, u"”"},        // ”
};

// Scan codes for the conjunct golden file's key labels (restated independently).
std::uint16_t scanForLetter(char lower) {
    static constexpr char kRows[3][11] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    static constexpr std::uint16_t kRowStart[3] = {0x10, 0x1E, 0x2C};
    for (int row = 0; row < 3; ++row) {
        const std::string_view keys = kRows[row];
        if (const auto pos = keys.find(lower); pos != std::string_view::npos)
            return static_cast<std::uint16_t>(kRowStart[row] + pos);
    }
    return 0;
}

std::u16string utf8ToUtf16(std::string_view s) {
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const auto b = static_cast<unsigned char>(s[i]);
        char32_t cp = 0;
        std::size_t len = 1;
        if (b < 0x80) cp = b;
        else if ((b >> 5) == 0x6) { cp = b & 0x1F; len = 2; }
        else if ((b >> 4) == 0xE) { cp = b & 0x0F; len = 3; }
        else return u"<invalid utf-8>";  // 4-byte sequences are not expected in this file
        if (i + len > s.size()) return u"<truncated utf-8>";
        for (std::size_t k = 1; k < len; ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        out.push_back(static_cast<char16_t>(cp));
        i += len;
    }
    return out;
}

} // namespace

TEST_CASE("every hand-checked Bijoy key maps to the expected token") {
    for (const Expect& e : kExpected) {
        const july::Token& t = july::lookupBijoyKey(e.scan, e.layer);
        const bool ok = t.kind == e.kind && t.text() == e.text;
        if (!ok) std::fprintf(stderr, "  mismatch at scan 0x%02X layer %d\n", e.scan, static_cast<int>(e.layer));
        CHECK(ok);
    }
}

TEST_CASE("layout table has exactly the rows the hand-checked list expects, plus unconfirmed ones") {
    std::size_t confirmed = 0;
    std::size_t unconfirmed = 0;
    for (const july::LayoutEntry& e : july::bijoyLayoutEntries()) {
        if (e.source == july::LayoutSource::Memory) ++unconfirmed;
        else ++confirmed;
    }
    // Every confirmed letter-row mapping must be covered by kExpected (digits are checked
    // separately below: 10 rows).
    CHECK(confirmed == std::size(kExpected) + 10);
    std::printf("  layout rows: %zu confirmed, %zu unconfirmed (source=memory)\n", confirmed, unconfirmed);
}

TEST_CASE("every row is on the master Bijoy chart") {
    // The master chart is the reference; nothing from another chart or layout may slip in.
    std::size_t master = 0;
    for (const july::LayoutEntry& e : july::bijoyLayoutEntries()) {
        const bool onMaster = e.source == july::LayoutSource::Master;
        if (!onMaster) std::fprintf(stderr, "  non-master row: scan 0x%02X layer %d\n", e.scan, static_cast<int>(e.layer));
        CHECK(onMaster);
        master += onMaster ? 1 : 0;
    }
    std::printf("  layout rows: %zu, all on the master chart\n", master);
}

TEST_CASE("digit keys produce Bengali digits") {
    // 1..9 = scan 0x02..0x0A -> U+09E7..U+09EF; 0 = scan 0x0B -> U+09E6
    for (std::uint16_t i = 0; i < 9; ++i) {
        const july::Token& t = july::lookupBijoyKey(static_cast<std::uint16_t>(0x02 + i), KeyLayer::Normal);
        CHECK(t.kind == TokenKind::Digit);
        CHECK(t.length == 1 && t.units[0] == static_cast<char16_t>(0x09E7 + i));
    }
    const july::Token& zero = july::lookupBijoyKey(0x0B, KeyLayer::Normal);
    CHECK(zero.kind == TokenKind::Digit && zero.units[0] == u'০');
}

TEST_CASE("unmapped and out-of-range keys return a None token") {
    CHECK(july::lookupBijoyKey(0x39, KeyLayer::Normal).kind == TokenKind::None);  // space
    CHECK(july::lookupBijoyKey(0x24, KeyLayer::Link).kind == TokenKind::None);    // g then j: no link form
    CHECK(july::lookupBijoyKey(0x2D, KeyLayer::Link).kind == TokenKind::None);   // ঔ is G+Shift+X, not G+X
    CHECK(july::lookupBijoyKey(0x22, KeyLayer::Link).kind == TokenKind::None);      // ॥ removed (not Bijoy)
    CHECK(july::lookupBijoyKey(0x22, KeyLayer::LinkShift).kind == TokenKind::None);
    CHECK(july::lookupBijoyKey(0x80, KeyLayer::Normal).kind == TokenKind::None);
    CHECK(july::lookupBijoyKey(0xFFFF, KeyLayer::Shift).kind == TokenKind::None);
    CHECK(july::lookupBijoyKey(0x24, static_cast<KeyLayer>(9)).kind == TokenKind::None);
}

namespace {

// Phase 2 stand-in for the composer: concatenates key tokens, except that the linker 'g'
// followed by a key with a link-layer form yields that form (e.g. g d -> ই).
int runGoldenFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    CHECK(in.good());
    int cases = 0;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto tab1 = line.find('\t');
        const auto tab2 = line.find('\t', tab1 + 1);
        CHECK(tab1 != std::string::npos && tab2 != std::string::npos);
        if (tab1 == std::string::npos || tab2 == std::string::npos) continue;

        const std::string keys = line.substr(0, tab1);
        const std::u16string expected = utf8ToUtf16(std::string_view(line).substr(tab1 + 1, tab2 - tab1 - 1));

        std::u16string actual;
        bool linkPending = false;
        for (const char c : keys) {
            if (c == ' ') continue;
            const bool shift = c >= 'A' && c <= 'Z';
            const char lower = shift ? static_cast<char>(c - 'A' + 'a') : c;
            const std::uint16_t scan = scanForLetter(lower);
            if (linkPending) {
                const july::Token& linked = july::lookupBijoyKey(scan, shift ? KeyLayer::LinkShift : KeyLayer::Link);
                linkPending = false;
                if (linked.kind != TokenKind::None) {
                    actual.pop_back();  // the linker's hasant is replaced by the linked form
                    actual += linked.text();
                    continue;
                }
            }
            const july::Token& t = july::lookupBijoyKey(scan, shift ? KeyLayer::Shift : KeyLayer::Normal);
            actual += t.text();
            linkPending = t.kind == TokenKind::Link;
        }
        const bool ok = actual == expected;
        if (!ok) std::fprintf(stderr, "  golden mismatch for keys '%s'\n", keys.c_str());
        CHECK(ok);
        ++cases;
    }
    return cases;
}

} // namespace

TEST_CASE("golden conjunct sequences produce the exact expected code points") {
    CHECK(runGoldenFile(JULY_GOLDEN_DIR "/bijoy_conjuncts.tsv") == 16);
}

TEST_CASE("golden linker-vowel sequences produce the exact expected code points") {
    CHECK(runGoldenFile(JULY_GOLDEN_DIR "/bijoy_link_vowels.tsv") == 11);
}
