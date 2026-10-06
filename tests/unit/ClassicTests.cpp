#include <windows.h>

#include <string>

#include "JulyTest.h"
#include "TestUtil.h"
#include "ClassicDiagnostics.h"
#include "july/engine/ClassicTable.h"
#include "july/engine/Composer.h"
#include "july/engine/Layout.h"

using july::Composer;
using july::ComposerOptions;
using july::NuktaForm;
using july::OutputEncoding;
using july::test::codepoints;

namespace {

constexpr ComposerOptions kClassic{NuktaForm::Precomposed, OutputEncoding::Classic};
constexpr char16_t kTaka = u'৳';  // known gap: no SutonnyMJ code yet

// True if every unit encodes to Windows-1252 exactly (no best-fit, no default char).
bool roundTripsCp1252(std::u16string_view text) {
    if (text.empty()) return true;
    BOOL usedDefault = FALSE;
    char bytes[256];
    const int n = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, reinterpret_cast<const wchar_t*>(text.data()),
                                      static_cast<int>(text.size()), bytes, sizeof bytes, nullptr, &usedDefault);
    return n == static_cast<int>(text.size()) && !usedDefault;
}

} // namespace

TEST_CASE("classic golden: Bijoy keys produce the exact SutonnyMJ glyph codes") {
    int cases = 0;
    for (const auto& g : july::test::readGolden(JULY_GOLDEN_DIR "/classic_sutonnymj.tsv")) {
        Composer composer(kClassic);
        const std::u16string actual = july::test::typeKeys(composer, g.keys);
        const bool ok = actual == g.expected;
        if (!ok) {
            std::fprintf(stderr, "  line %d keys '%s': expected [%s] got [%s]\n", g.line, g.keys.c_str(),
                         codepoints(g.expected).c_str(), codepoints(actual).c_str());
        }
        CHECK(ok);
        CHECK(roundTripsCp1252(actual));
        ++cases;
    }
    CHECK(cases == 25);
}

TEST_CASE("classic: every glyph in the table round-trips through code page 1252") {
    for (const july::ClassicEntry& e : july::sutonnyEntries()) {
        const bool ok = roundTripsCp1252(e.glyphText());
        if (!ok) std::fprintf(stderr, "  not cp1252: key [%s]\n", codepoints(e.keyText()).c_str());
        CHECK(ok);
    }
}

TEST_CASE("classic: the table is sorted and has no duplicate keys (binary search precondition)") {
    const auto table = july::sutonnyEntries();
    for (std::size_t i = 1; i < table.size(); ++i) CHECK(table[i - 1].keyText() < table[i].keyText());
}

TEST_CASE("classic: every character the layout can type has a glyph, except the known ৳ gap") {
    std::size_t gaps = 0;
    for (const july::LayoutEntry& e : july::bijoyLayoutEntries()) {
        july::TextBuffer<16> out;
        const std::size_t unmapped = july::appendSutonny(e.token.text(), out);
        if (unmapped == 0) continue;
        const bool knownGap = e.token.length == 1 && e.token.units[0] == kTaka;
        if (!knownGap) std::fprintf(stderr, "  no classic glyph for [%s]\n", codepoints(e.token.text()).c_str());
        CHECK(knownGap);
        ++gaps;
    }
    CHECK(gaps == 1);
}

TEST_CASE("classic: digits and dari are converted when committed directly") {
    Composer c(kClassic);
    CHECK(c.pressKey(0x02, false).commit.view() == u"1");  // ১
    CHECK(c.pressKey(0x22, true).commit.view() == u"|");   // Shift+G = ।
}

TEST_CASE("classic: Backspace re-renders the syllable in glyph codes") {
    Composer c(kClassic);
    (void)c.pressKey(july::test::scanForLetter('j'), false);
    (void)c.pressKey(july::test::scanForLetter('g'), false);
    auto r = c.pressKey(july::test::scanForLetter('n'), true);  // ক্ষ
    CHECK(r.composition.view() == u"¶");                    // ¶
    r = c.backspace();
    CHECK(r.composition.view() == u"K&");                         // ক্
}

TEST_CASE("classic: table provenance is reported (verification pending)") {
    std::size_t verified = 0;
    for (const july::ClassicEntry& e : july::sutonnyEntries()) verified += e.verified ? 1 : 0;
    std::printf("  classic table: %zu rows, %zu verified in SutonnyMJ, %zu from converter\n",
                july::sutonnyEntries().size(), verified, july::sutonnyEntries().size() - verified);
    CHECK(july::sutonnyEntries().size() == 222);
}

TEST_CASE("classic diagnostics report the ANSI code page and SutonnyMJ availability") {
    const july::ClassicDiagnostics d = july::checkClassicEnvironment();
    CHECK(d.ansiCodePage == GetACP());
    CHECK(d.ansiCodePageOk == (d.ansiCodePage == 1252));
    std::printf("  classic environment: ANSI code page %u (%s), SutonnyMJ %s\n", d.ansiCodePage,
                d.ansiCodePageOk ? "ok" : "Classic text will not round-trip in ANSI apps",
                d.sutonnyInstalled ? "installed" : "NOT installed");
}
