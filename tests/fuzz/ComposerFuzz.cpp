// Deterministic fuzzer for the Bijoy composer.
//
//   july_fuzz [seed] [events]
//
// Drives random keys (every scan code, mapped or not, with random Shift), backspace,
// commit (focus change / navigation) and reset (application ended the composition), and
// checks invariants after every step:
//   - output buffers never exceed capacity and never contain lone surrogates
//   - composing() agrees with the returned composition text, and is false after
//     commitAll() or reset()
//   - backspacing an open syllable always empties it within kMaxSyllableKeys + 1 steps
//   - the whole run is deterministic: the same seed twice gives the same output hash
// Exits 0 on success, 1 on the first violated invariant.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "july/engine/Composer.h"

namespace {

struct Rng {  // xorshift64*: tiny, deterministic, good enough for fuzzing
    std::uint64_t state;
    std::uint64_t next() noexcept {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DULL;
    }
    std::uint32_t below(std::uint32_t n) noexcept { return static_cast<std::uint32_t>(next() % n); }
};

struct Hash {  // FNV-1a over all emitted UTF-16 units
    std::uint64_t value = 0xcbf29ce484222325ULL;
    void add(std::u16string_view text) noexcept {
        for (char16_t c : text) {
            value ^= c;
            value *= 0x100000001b3ULL;
        }
        value ^= 0xFFFF;  // separator so "ab"+"" differs from "a"+"b"
        value *= 0x100000001b3ULL;
    }
};

std::uint64_t g_step = 0;

bool fail(const char* what) {
    std::fprintf(stderr, "invariant violated at event %llu: %s\n", static_cast<unsigned long long>(g_step), what);
    return false;
}

bool checkText(std::u16string_view text) {
    if (text.size() > july::kTextCapacity) return fail("text exceeds capacity");
    for (char16_t c : text) {
        if (c >= 0xD800 && c <= 0xDFFF) return fail("surrogate code unit in output");
        if (c < 0x20) return fail("control character in output");
    }
    return true;
}

bool checkResult(const july::EditResult& r, const july::Composer& c) {
    if (!checkText(r.commit.view()) || !checkText(r.composition.view())) return false;
    if (r.composition.empty() != !c.composing()) return fail("composing() disagrees with composition text");
    return true;
}

// Returns the output hash, or 0 on failure.
std::uint64_t run(std::uint64_t seed, std::uint64_t events) {
    Rng rng{seed ? seed : 1};
    Hash hash;
    july::Composer composer;
    for (g_step = 0; g_step < events; ++g_step) {
        const std::uint32_t action = rng.below(100);
        july::EditResult r;
        if (action < 80) {
            const auto scan = static_cast<std::uint16_t>(rng.below(0x90));  // includes out-of-range
            r = composer.pressKey(scan, rng.below(3) == 0);
        } else if (action < 92) {
            r = composer.backspace();
        } else if (action < 96) {
            r = composer.commitAll();
            if (composer.composing()) return fail("still composing after commitAll"), 0;
        } else if (action < 98) {
            composer.reset();
            if (composer.composing()) return fail("still composing after reset"), 0;
            continue;
        } else {
            // Backspace the open syllable away; must terminate within the key capacity.
            std::size_t steps = 0;
            while (composer.composing()) {
                r = composer.backspace();
                if (!checkResult(r, composer)) return 0;
                if (++steps > july::kMaxSyllableKeys + 1) return fail("backspace did not empty the syllable"), 0;
            }
            if (composer.backspace().eaten) return fail("backspace eaten with nothing composing"), 0;
            continue;
        }
        if (!checkResult(r, composer)) return 0;
        hash.add(r.commit.view());
        hash.add(r.composition.view());
    }
    return hash.value;
}

} // namespace

int main(int argc, char** argv) {
    const std::uint64_t seed = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 20261006;
    const std::uint64_t events = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 1'000'000;

    const auto start = std::chrono::steady_clock::now();
    const std::uint64_t first = run(seed, events);
    if (first == 0) return 1;
    const std::uint64_t second = run(seed, events);
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (second != first) {
        std::fprintf(stderr, "non-deterministic: hash %016llx vs %016llx\n",
                     static_cast<unsigned long long>(first), static_cast<unsigned long long>(second));
        return 1;
    }
    std::printf("july_fuzz: seed %llu, %llu events x2, deterministic (hash %016llx), %.2f s\n",
                static_cast<unsigned long long>(seed), static_cast<unsigned long long>(events),
                static_cast<unsigned long long>(first), elapsed);
    return 0;
}
