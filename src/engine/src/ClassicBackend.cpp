// Classic backend: renders a syllable as SutonnyMJ glyph codes in visual order.
//
//   output = [independent vowel glyphs]
//            [pre-base kar glyph]            ি ে ৈ (and the ে half of ো ৌ) come first
//            cluster + phala glyphs          longest match, e.g. ক্ষ -> ¶, ক্র -> µ
//            [reph glyph ©]                  after the cluster it sits on
//            post-base kar glyph             া ী ু …, or the া / ৗ half of ো ৌ
//            modifier glyphs                 ং ঃ ঁ
//
// Without a reph, cluster + post-kar are matched together so forms such as গু (¸),
// রু (i“) and হৃ (ü) are used.

#include <algorithm>

#include "july/engine/ClassicTable.h"
#include "july/engine/Syllable.h"

namespace july {

namespace {

#include "july/engine/generated/ClassicTable.inc"

constexpr char16_t kHasant = u'্';
constexpr char16_t kRa = u'র';
constexpr char16_t kYa = u'য';
constexpr char16_t kZwj = u'‍';
constexpr char16_t kReph[] = {kRa, kHasant};

constexpr std::size_t kMaxKeyLength = 6;

} // namespace

std::span<const ClassicEntry> sutonnyEntries() noexcept {
    return kSutonnyEntries;
}

const ClassicEntry* longestSutonnyMatch(std::u16string_view text) noexcept {
    const std::span<const ClassicEntry> table = kSutonnyEntries;
    for (std::size_t len = std::min(text.size(), kMaxKeyLength); len > 0; --len) {
        const std::u16string_view prefix = text.substr(0, len);
        const auto it = std::lower_bound(table.begin(), table.end(), prefix,
                                         [](const ClassicEntry& e, std::u16string_view k) { return e.keyText() < k; });
        if (it != table.end() && it->keyText() == prefix) return &*it;
    }
    return nullptr;
}

void renderClassic(std::span<const KeyToken> keys, EngineText& out) noexcept {
    EngineText leading;
    EngineText core;  // cluster + phalas, logical Unicode
    EngineText modifiers;
    bool reph = false;
    char16_t pre = 0;
    char16_t post = 0;
    std::size_t consonants = 0;
    bool clusterHasLink = false;

    for (const KeyToken& key : keys) {
        const Token& t = key.token;
        switch (t.kind) {
        case TokenKind::Consonant:    core.push(t.units[0]); ++consonants; break;
        case TokenKind::Link:         core.push(kHasant); clusterHasLink = true; break;
        case TokenKind::Phala:
            // র + ্য without reph is র‍্য (ZWJ), matching the Unicode backend and the table.
            if (!reph && consonants == 1 && !clusterHasLink && core.view() == std::u16string_view(&kRa, 1) &&
                t.length == 2 && t.units[1] == kYa) {
                core.push(kZwj);
            }
            core.append(t.text());
            break;
        case TokenKind::VowelSignPre: pre = t.units[0]; break;
        case TokenKind::VowelSign:    post = t.units[0]; break;
        case TokenKind::Reph:         reph = true; break;
        case TokenKind::Modifier:     modifiers.append(t.text()); break;
        default:                      leading.append(t.text()); break;
        }
    }

    appendSutonny(leading.view(), out);

    // ো / ৌ need no special case: the composer keeps them as ে (pre) + া / ৗ (post), which is
    // exactly how Bijoy Classic stores them (ে before the cluster, া / ৗ after it).
    if (pre != 0) appendSutonny(std::u16string_view(&pre, 1), out);

    if (reph) {
        appendSutonny(core.view(), out);
        appendSutonny(std::u16string_view(kReph, 2), out);
        if (post != 0) appendSutonny(std::u16string_view(&post, 1), out);
    } else {
        if (post != 0) core.push(post);
        appendSutonny(core.view(), out);
    }
    appendSutonny(modifiers.view(), out);
}

} // namespace july
