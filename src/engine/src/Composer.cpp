#include "july/engine/Composer.h"

#include <span>

#include "july/engine/Layout.h"

namespace july {

namespace {

constexpr char16_t kSignE = u'ে';         // ে
constexpr char16_t kSignAa = u'া';        // া
constexpr char16_t kAuLengthMark = u'ৗ';  // ৗ
constexpr Token kLinkToken{TokenKind::Link, 1, {u'্'}};

// Structural summary of the open syllable, recomputed from the key list (at most 16
// entries) so it is always consistent with Backspace.
struct Shape {
    bool pre = false;
    bool cluster = false;      // at least one consonant
    bool post = false;
    bool reph = false;
    bool independent = false;
    bool endsWithLink = false;
    std::uint8_t phalas = 0;
    std::uint8_t modifiers = 0;
    char16_t preUnit = 0;
};

Shape analyze(std::span<const KeyToken> keys) noexcept {
    Shape s;
    for (const KeyToken& key : keys) {
        switch (key.token.kind) {
        case TokenKind::Consonant:        s.cluster = true; break;
        case TokenKind::VowelSignPre:     s.pre = true; s.preUnit = key.token.units[0]; break;
        case TokenKind::VowelSign:        s.post = true; break;
        case TokenKind::Reph:             s.reph = true; break;
        case TokenKind::IndependentVowel: s.independent = true; break;
        case TokenKind::Phala:            ++s.phalas; break;
        case TokenKind::Modifier:         ++s.modifiers; break;
        default:                          break;
        }
    }
    s.endsWithLink = !keys.empty() && keys.back().token.kind == TokenKind::Link;
    return s;
}

} // namespace

bool Composer::accepts(const Token& token) const noexcept {
    if (count_ == 0 || count_ == kMaxSyllableKeys) return false;
    const Shape s = analyze(std::span(keys_.data(), count_));

    switch (token.kind) {
    case TokenKind::Consonant:
        // Joins after the linker (conjunct), or attaches to a pending pre-base kar.
        return (s.cluster && s.endsWithLink) || (s.pre && !s.cluster);
    case TokenKind::Link:
        return s.cluster && !s.endsWithLink && !s.post && s.phalas == 0 && !s.reph && s.modifiers == 0;
    case TokenKind::Phala:
        return s.cluster && !s.endsWithLink && !s.post && s.modifiers == 0 && s.phalas < 2;
    case TokenKind::VowelSign:
        if (!s.cluster || s.endsWithLink || s.post) return false;
        // A pre-base kar only combines with the second half of a split vowel: ে + া = ো,
        // ে + ৗ = ৌ.
        return !s.pre || (s.preUnit == kSignE && (token.units[0] == kSignAa || token.units[0] == kAuLengthMark));
    case TokenKind::Reph:
        // Typed after the cluster it sits on (and after that cluster's post-base kar).
        return s.cluster && !s.endsWithLink && !s.reph && s.modifiers == 0;
    case TokenKind::Modifier:
        return (s.cluster || s.independent) && !s.endsWithLink && s.modifiers < 2;
    default:
        // Pre-base kars and independent vowels always start a new syllable.
        return false;
    }
}

void Composer::push(const Token& token, bool consumedLink) noexcept {
    if (count_ < kMaxSyllableKeys) keys_[count_++] = KeyToken{token, consumedLink};
}

void Composer::render(EngineText& out) const noexcept {
    renderUnicode(std::span(keys_.data(), count_), options_, out);
}

void Composer::commitInto(EditResult& result) noexcept {
    render(result.commit);
    count_ = 0;
}

void Composer::finish(EditResult& result) const noexcept {
    result.composition.clear();
    render(result.composition);
}

EditResult Composer::pressKey(std::uint16_t scan, bool shift) noexcept {
    EditResult result;
    result.eaten = true;

    // 'g' followed by a key that has a linked form (g + d = ই): the linker is consumed.
    if (count_ > 0 && keys_[count_ - 1].token.kind == TokenKind::Link) {
        const Token& linked = lookupBijoyKey(scan, shift ? KeyLayer::LinkShift : KeyLayer::Link);
        if (linked.kind != TokenKind::None) {
            --count_;
            commitInto(result);
            push(linked, /*consumedLink=*/true);
            finish(result);
            return result;
        }
    }

    const Token& token = lookupBijoyKey(scan, shift ? KeyLayer::Shift : KeyLayer::Normal);
    switch (token.kind) {
    case TokenKind::None:
        // Not a Bijoy key (space, Enter, unmapped symbol): finalize and let it through.
        commitInto(result);
        result.eaten = false;
        break;
    case TokenKind::Digit:
    case TokenKind::Punct:
    case TokenKind::Final:
        // Never part of a syllable and never modified later: emit directly.
        commitInto(result);
        result.commit.append(token.text());
        break;
    default:
        if (!accepts(token)) commitInto(result);
        push(token, /*consumedLink=*/false);
        break;
    }
    finish(result);
    return result;
}

EditResult Composer::backspace() noexcept {
    EditResult result;
    if (count_ == 0) return result;  // not eaten: the application deletes

    const KeyToken last = keys_[--count_];
    if (last.consumedLink) push(kLinkToken, /*consumedLink=*/false);
    result.eaten = true;
    finish(result);
    return result;
}

EditResult Composer::commitAll() noexcept {
    EditResult result;
    commitInto(result);
    return result;
}

} // namespace july
