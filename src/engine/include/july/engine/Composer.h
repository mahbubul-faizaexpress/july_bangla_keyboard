#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "july/engine/Syllable.h"

namespace july {

// What the host (TSF text service) must do after a key:
//   1. replace the current composition with `commit` + `composition`,
//   2. finalize `commit` (it leaves the composition for good),
//   3. keep `composition` as the new composition (empty = end the composition).
// If `eaten` is false, the host must also pass the original key to the application
// (after doing 1-3, so committed text lands before the key's own effect).
struct EditResult {
    bool eaten = false;
    EngineText commit;
    EngineText composition;
};

// Bijoy composer: a deterministic state machine over logical key tokens.
//
// Typing is in Bijoy visual order: pre-base kars (ি ে ৈ) before their consonant cluster,
// the reph key after the cluster it sits on, 'g' links consonants, and 'g' + a vowel-sign
// key gives the independent vowel. The open syllable is kept as the list of typed keys,
// and its text is re-rendered from that list after every change, so Backspace undoes
// exactly one key and never relies on counting characters.
//
// Not thread-safe by design: one instance per TSF thread. No heap allocation.
class Composer {
public:
    explicit Composer(ComposerOptions options = {}) noexcept : options_(options) {}

    // A Bijoy key (scan code + Shift). Ctrl/Alt chords and Caps Lock are the host's
    // concern: Caps Lock is ignored by design, so pass only the Shift state.
    [[nodiscard]] EditResult pressKey(std::uint16_t scan, bool shift) noexcept;

    // Undo the last key of the open syllable. Not eaten when nothing is composing, so the
    // application deletes with its own rules.
    [[nodiscard]] EditResult backspace() noexcept;

    // Finalize the open syllable (navigation keys, focus change, mode switch).
    [[nodiscard]] EditResult commitAll() noexcept;

    // Drop the open syllable without emitting it (the application ended the
    // composition itself, or an error path).
    void reset() noexcept { count_ = 0; }

    [[nodiscard]] bool composing() const noexcept { return count_ != 0; }

private:
    void render(EngineText& out) const noexcept;
    void commitInto(EditResult& result) noexcept;
    void finish(EditResult& result) const noexcept;
    bool accepts(const Token& token) const noexcept;
    void push(const Token& token, bool consumedLink) noexcept;

    std::array<KeyToken, kMaxSyllableKeys> keys_{};
    std::size_t count_ = 0;
    ComposerOptions options_;
};

} // namespace july
