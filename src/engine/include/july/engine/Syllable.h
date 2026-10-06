#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "july/engine/TextBuffer.h"
#include "july/engine/Token.h"

namespace july {

// One typed key inside the open syllable, in typing (visual) order.
struct KeyToken {
    Token token;
    bool consumedLink = false;  // produced from 'g' + key; Backspace restores the 'g'
};

inline constexpr std::size_t kMaxSyllableKeys = 16;

// Large enough for a full syllable (16 keys x 3 units + reph + ZWJ) plus one committed
// digit or sign.
inline constexpr std::size_t kTextCapacity = 96;
using EngineText = TextBuffer<kTextCapacity>;

// How ড় ঢ় য় are emitted. Precomposed = U+09DC/U+09DD/U+09DF. Decomposed = base letter +
// nukta U+09BC, which is their NFC form (the three are Unicode composition exclusions).
enum class NuktaForm : std::uint8_t { Precomposed, Decomposed };

// Output backend: the same typed syllable rendered as Unicode or as Bijoy Classic
// (SutonnyMJ glyph codes, cp1252-compatible).
enum class OutputEncoding : std::uint8_t { Unicode, Classic };

struct ComposerOptions {
    NuktaForm nukta = NuktaForm::Precomposed;  // Unicode output only
    OutputEncoding encoding = OutputEncoding::Unicode;
};

// Renders a syllable's keys as logical-order Unicode (Unicode backend).
void renderUnicode(std::span<const KeyToken> keys, const ComposerOptions& options,
                   EngineText& out) noexcept;

// Renders a syllable's keys as SutonnyMJ glyph codes in visual order (Classic backend).
void renderClassic(std::span<const KeyToken> keys, EngineText& out) noexcept;

} // namespace july
