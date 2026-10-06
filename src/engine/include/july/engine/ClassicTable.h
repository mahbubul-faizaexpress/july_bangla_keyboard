#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "july/engine/TextBuffer.h"

namespace july {

// One SutonnyMJ mapping: a logical Unicode sequence and its glyph code units (the
// cp1252-decoded form of the legacy bytes).
struct ClassicEntry {
    char16_t key[6];
    std::uint8_t keyLength;
    char16_t glyph[4];
    std::uint8_t glyphLength;
    bool verified;  // rendered in SutonnyMJ and checked; false = taken from a converter

    [[nodiscard]] constexpr std::u16string_view keyText() const noexcept { return {key, keyLength}; }
    [[nodiscard]] constexpr std::u16string_view glyphText() const noexcept { return {glyph, glyphLength}; }
};

// The whole table, sorted by key (for tests and diagnostics).
[[nodiscard]] std::span<const ClassicEntry> sutonnyEntries() noexcept;

// Converts logical Unicode to SutonnyMJ glyph code units by greedy longest match and
// appends it to `out`. A code point with no mapping is appended unchanged, so text is
// never dropped. Returns the number of unmapped code points.
template <std::size_t N>
std::size_t appendSutonny(std::u16string_view text, TextBuffer<N>& out) noexcept;

// Non-template core used by appendSutonny: longest entry whose key is a prefix of `text`,
// or nullptr.
[[nodiscard]] const ClassicEntry* longestSutonnyMatch(std::u16string_view text) noexcept;

template <std::size_t N>
std::size_t appendSutonny(std::u16string_view text, TextBuffer<N>& out) noexcept {
    std::size_t unmapped = 0;
    while (!text.empty()) {
        if (const ClassicEntry* e = longestSutonnyMatch(text)) {
            out.append(e->glyphText());
            text.remove_prefix(e->keyLength);
        } else {
            out.push(text.front());
            text.remove_prefix(1);
            ++unmapped;
        }
    }
    return unmapped;
}

} // namespace july
