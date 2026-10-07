#include "july/engine/InputMode.h"

namespace july {

InputMode modeFromInt(std::int32_t value) noexcept {
    switch (value) {
    case static_cast<std::int32_t>(InputMode::Unicode): return InputMode::Unicode;
    case static_cast<std::int32_t>(InputMode::Classic): return InputMode::Classic;
    default:                                            return InputMode::English;
    }
}

std::u8string_view modeLabel(InputMode m) noexcept {
    switch (m) {
    // Escapes keep the exact code points unambiguous (য় is written NFC: য + ়).
    case InputMode::Unicode: return u8"বাংলা";        // বাংলা
    case InputMode::Classic: return u8"ক্লাসিক";      // ক্লাসিক
    case InputMode::English: break;
    }
    return u8"EN";
}

} // namespace july
