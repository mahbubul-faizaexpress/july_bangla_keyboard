#include <string>

#include "JulyTest.h"
#include "TestUtil.h"
#include "july/engine/Composer.h"

using july::Composer;
using july::ComposerOptions;
using july::EditResult;
using july::NuktaForm;
using july::test::codepoints;
using july::test::scanForLetter;

namespace {

constexpr std::uint16_t kScanSpace = 0x39;
constexpr std::uint16_t kScan1 = 0x02;
constexpr std::uint16_t kScan7 = 0x08;
constexpr std::uint16_t kScanBackslash = 0x2B;

EditResult press(Composer& c, char label) {
    const bool shift = label >= 'A' && label <= 'Z';
    return c.pressKey(scanForLetter(shift ? static_cast<char>(label - 'A' + 'a') : label), shift);
}

bool expectText(std::u16string_view actual, std::u16string_view expected, const char* what) {
    if (actual == expected) return true;
    std::fprintf(stderr, "  %s: expected [%s] got [%s]\n", what, codepoints(expected).c_str(),
                 codepoints(actual).c_str());
    return false;
}

int runGolden(const char* path) {
    int cases = 0;
    for (const auto& g : july::test::readGolden(path)) {
        Composer composer;
        const std::u16string actual = july::test::typeKeys(composer, g.keys);
        const bool ok = actual == g.expected;
        if (!ok) {
            std::fprintf(stderr, "  %s(%d) keys '%s': expected [%s] got [%s]\n", path, g.line, g.keys.c_str(),
                         codepoints(g.expected).c_str(), codepoints(actual).c_str());
        }
        CHECK(ok);
        CHECK(!composer.composing());
        ++cases;
    }
    return cases;
}

} // namespace

// --- Golden files ------------------------------------------------------------------------

TEST_CASE("golden: conjuncts (user-supplied, master chart rules)") {
    CHECK(runGolden(JULY_GOLDEN_DIR "/bijoy_conjuncts.tsv") == 16);
}

TEST_CASE("golden: independent vowels via the linker G (master rules table)") {
    CHECK(runGolden(JULY_GOLDEN_DIR "/bijoy_link_vowels.tsv") == 11);
}

TEST_CASE("golden: Bijoy visual typing order (pre-base kar first, reph after)") {
    CHECK(runGolden(JULY_GOLDEN_DIR "/bijoy_typing_order.tsv") == 13);
}

// --- Composition behaviour -----------------------------------------------------------------

TEST_CASE("a consonant that cannot join commits the previous syllable") {
    Composer c;
    EditResult r = press(c, 'j');
    CHECK(r.eaten && r.commit.empty());
    CHECK(expectText(r.composition.view(), u"ক", "after j"));  // ক
    r = press(c, 'm');
    CHECK(expectText(r.commit.view(), u"ক", "commit on m"));
    CHECK(expectText(r.composition.view(), u"ম", "composition on m"));  // ম
}

TEST_CASE("backspace undoes exactly one key, then falls through to the application") {
    Composer c;
    (void)press(c, 'j');
    (void)press(c, 'g');
    EditResult r = press(c, 'N');
    CHECK(expectText(r.composition.view(), u"ক্ষ", "ক্ষ"));
    r = c.backspace();
    CHECK(r.eaten && r.commit.empty());
    CHECK(expectText(r.composition.view(), u"ক্", "after 1st backspace"));
    r = c.backspace();
    CHECK(expectText(r.composition.view(), u"ক", "after 2nd backspace"));
    r = c.backspace();
    CHECK(r.eaten && r.composition.empty() && !c.composing());
    r = c.backspace();
    CHECK(!r.eaten);  // nothing composing: the application deletes
}

TEST_CASE("backspace after G + vowel restores the linker") {
    Composer c;
    (void)press(c, 'g');
    EditResult r = press(c, 'd');
    CHECK(expectText(r.composition.view(), u"ই", "ই"));
    r = c.backspace();
    CHECK(expectText(r.composition.view(), u"্", "linker restored"));
    r = press(c, 'f');
    CHECK(expectText(r.composition.view(), u"আ", "আ after re-linking"));
}

TEST_CASE("G + vowel key after a consonant gives an independent vowel, not a sign") {
    Composer c;
    CHECK(july::test::typeKeys(c, "j g d") == u"কই");  // কই
}

TEST_CASE("ra + ya-phala gets ZWJ (র‍্য); reph + য does not (র্য)") {
    Composer c;
    CHECK(expectText(july::test::typeKeys(c, "v Z"), u"র‍্য", "র‍্য"));
    CHECK(expectText(july::test::typeKeys(c, "w A"), u"র্য", "র্য"));
}

TEST_CASE("ra-phala and ya-phala follow the consonant without G") {
    Composer c;
    CHECK(expectText(july::test::typeKeys(c, "j z"), u"ক্র", "ক্র"));
    CHECK(expectText(july::test::typeKeys(c, "h Z f"), u"ব্যা", "ব্যা"));
    CHECK(expectText(july::test::typeKeys(c, "c j z f"), u"ক্রো", "ক্রো"));
}

TEST_CASE("modifiers follow the vowel sign in logical order") {
    Composer c;
    CHECK(expectText(july::test::typeKeys(c, "h f Q"), u"বাং", "বাং"));
    (void)press(c, 'j');
    (void)press(c, 'f');
    EditResult r = c.pressKey(kScan7, true);  // Shift+7 = ঁ
    CHECK(expectText(r.composition.view(), u"কাঁ", "কাঁ"));
    (void)c.commitAll();
}

TEST_CASE("space and unmapped keys commit and pass through") {
    Composer c;
    (void)press(c, 'j');
    const EditResult r = c.pressKey(kScanSpace, false);
    CHECK(!r.eaten);
    CHECK(expectText(r.commit.view(), u"ক", "commit on space"));
    CHECK(r.composition.empty() && !c.composing());
}

TEST_CASE("digits, dari and khanda-ta are committed immediately") {
    Composer c;
    (void)press(c, 'j');
    EditResult r = c.pressKey(kScan1, false);
    CHECK(r.eaten && r.composition.empty());
    CHECK(expectText(r.commit.view(), u"ক১", "ক১"));
    r = press(c, 'G');
    CHECK(expectText(r.commit.view(), u"।", "।"));
    (void)press(c, 'k');
    r = c.pressKey(kScanBackslash, false);
    CHECK(expectText(r.commit.view(), u"তৎ", "তৎ"));
}

TEST_CASE("a pre-base kar with no consonant is committed as typed") {
    Composer c;
    (void)press(c, 'd');
    const EditResult r = c.pressKey(kScanSpace, false);
    CHECK(expectText(r.commit.view(), u"ি", "dangling ি"));
}

TEST_CASE("an i-kar does not combine with aa-kar; the aa-kar starts a new syllable") {
    Composer c;
    CHECK(expectText(july::test::typeKeys(c, "d j f"), u"কিা", "কি + া"));
}

TEST_CASE("nukta letters: precomposed by default, NFC-decomposed on request") {
    Composer precomposed;
    CHECK(expectText(july::test::typeKeys(precomposed, "W p P"), u"য়ড়ঢ়", "য় ড় ঢ় precomposed"));
    Composer decomposed(ComposerOptions{NuktaForm::Decomposed});
    CHECK(expectText(july::test::typeKeys(decomposed, "W p P"),
                     u"য়ড়ঢ়", "য় ড় ঢ় decomposed"));
}

TEST_CASE("commitAll and reset end the composition") {
    Composer c;
    (void)press(c, 'j');
    EditResult r = c.commitAll();
    CHECK(expectText(r.commit.view(), u"ক", "commitAll"));
    CHECK(!c.composing());
    (void)press(c, 'j');
    c.reset();
    CHECK(!c.composing());
    CHECK(c.commitAll().commit.empty());
}

TEST_CASE("a syllable never grows past its fixed capacity") {
    Composer c;
    std::u16string text;
    for (int i = 0; i < 40; ++i) {
        EditResult r = press(c, i % 2 == 0 ? 'j' : 'g');
        CHECK(r.composition.size() <= july::kTextCapacity);
        text += r.commit.view();
    }
    text += c.commitAll().commit.view();
    // 20 consonants joined by 19 links (plus a trailing one), split across syllables.
    std::size_t consonants = 0;
    for (char16_t ch : text) consonants += ch == u'ক' ? 1 : 0;
    CHECK(consonants == 20);
}

TEST_CASE("অ then া becomes আ, and Backspace returns to অ") {
    Composer c;
    EditResult r = press(c, 'F');
    CHECK(expectText(r.composition.view(), u"\u0985", "অ"));
    r = press(c, 'f');
    CHECK(expectText(r.composition.view(), u"\u0986", "আ"));
    r = c.backspace();
    CHECK(expectText(r.composition.view(), u"\u0985", "back to অ"));
    // Other vowel signs after অ do not combine: the sign starts a new syllable.
    r = press(c, 'd');
    CHECK(expectText(r.commit.view(), u"\u0985", "অ committed before ি"));
    (void)c.commitAll();
}
