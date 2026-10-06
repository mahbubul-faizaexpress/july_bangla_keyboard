// Unicode backend: turns a syllable typed in Bijoy visual order into logical-order Unicode.
//
//   output = [independent vowel]
//            [র ্ if reph] cluster [ZWJ for র‍্য] phalas vowel-sign modifiers
//
// Shaping (conjunct glyphs, reph position, kar placement) is left to the font.

#include "july/engine/Syllable.h"

namespace july {

namespace {

constexpr char16_t kHasant = u'্';
constexpr char16_t kRa = u'র';
constexpr char16_t kYa = u'য';
constexpr char16_t kNukta = u'়';
constexpr char16_t kZwj = u'‍';
constexpr char16_t kSignE = u'ে';         // ে
constexpr char16_t kSignAa = u'া';        // া
constexpr char16_t kAuLengthMark = u'ৗ';  // ৗ
constexpr char16_t kSignO = u'ো';         // ো (NFC of ে + া)
constexpr char16_t kSignAu = u'ৌ';        // ৌ (NFC of ে + ৗ)

void appendConsonant(char16_t c, NuktaForm nukta, TextBuffer<kTextCapacity>& out) noexcept {
    if (nukta == NuktaForm::Decomposed) {
        switch (c) {
        case u'ড়': out.push(u'ড'); out.push(kNukta); return;  // ড়
        case u'ঢ়': out.push(u'ঢ'); out.push(kNukta); return;  // ঢ়
        case u'য়': out.push(kYa); out.push(kNukta); return;        // য়
        default: break;
        }
    }
    out.push(c);
}

} // namespace

void renderUnicode(std::span<const KeyToken> keys, const ComposerOptions& options,
                   EngineText& out) noexcept {
    EngineText leading;   // independent vowels, finals, stray signs
    EngineText cluster;   // consonants and hasants, in typed order
    EngineText phalas;
    EngineText modifiers;
    bool reph = false;
    char16_t pre = 0;
    char16_t post = 0;
    std::size_t consonants = 0;
    bool clusterHasLink = false;

    for (const KeyToken& key : keys) {
        const Token& t = key.token;
        switch (t.kind) {
        case TokenKind::Consonant:
            appendConsonant(t.units[0], options.nukta, cluster);
            ++consonants;
            break;
        case TokenKind::Link:
            cluster.push(kHasant);
            clusterHasLink = true;
            break;
        case TokenKind::Phala:        phalas.append(t.text()); break;
        case TokenKind::VowelSignPre: pre = t.units[0]; break;
        case TokenKind::VowelSign:    post = t.units[0]; break;
        case TokenKind::Reph:         reph = true; break;
        case TokenKind::Modifier:     modifiers.append(t.text()); break;
        default:                      leading.append(t.text()); break;
        }
    }

    out.append(leading.view());
    if (reph) {
        out.push(kRa);
        out.push(kHasant);
    }
    out.append(cluster.view());
    // র + ্য without a reph is the ra + ya-phala form (র‍্য), which needs ZWJ; without it
    // the sequence would render as reph + য.
    if (!reph && consonants == 1 && !clusterHasLink && cluster.view() == std::u16string_view(&kRa, 1) &&
        phalas.size() >= 2 && phalas.view()[1] == kYa) {
        out.push(kZwj);
    }
    out.append(phalas.view());
    if (pre == kSignE && post == kSignAa) {
        out.push(kSignO);
    } else if (pre == kSignE && post == kAuLengthMark) {
        out.push(kSignAu);
    } else {
        if (pre != 0) out.push(pre);
        if (post != 0) out.push(post);
    }
    out.append(modifiers.view());
}

} // namespace july
