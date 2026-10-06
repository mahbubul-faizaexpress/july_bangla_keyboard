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
