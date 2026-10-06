#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace july {

// Logical class of what a key produces. The composer (Phase 3) works on these classes;
// the exact code points live in the layout data.
enum class TokenKind : std::uint8_t {
    None,              // key not mapped in this layer
    Consonant,         // ক খ … (may take hasant, phala, vowel signs)
    Final,             // ৎ: a consonant form that takes nothing after it
    Link,              // Bijoy linker 'g' (hasant U+09CD)
    VowelSign,         // post-base / below-base: া ী ু ূ ৃ ৗ
    VowelSignPre,      // pre-base, typed before the consonant in Bijoy: ি ে ৈ
    IndependentVowel,  // অ আ ই … ও ঔ
    Modifier,          // ং ঃ ঁ
    Phala,             // ্র ্য (hasant + consonant as one key)
    Reph,              // র্ (র + hasant as one key)
    Digit,             // ০–৯
    Punct,             // । ৳ …
};

// Hot-path token: 8 bytes, trivially copyable, no heap.
struct Token {
    TokenKind kind = TokenKind::None;
    std::uint8_t length = 0;  // number of UTF-16 units used in `units`
    char16_t units[3] = {};

    [[nodiscard]] constexpr std::u16string_view text() const noexcept {
        return {units, length};
    }
};

static_assert(sizeof(Token) == 8);
static_assert(std::is_trivially_copyable_v<Token>);

} // namespace july
