#pragma once

#include <cstdint>
#include <string_view>

namespace july {

enum class InputMode : std::uint8_t {
    English,
    Unicode,
    Classic,
};

// Ctrl+Alt+B cycle order: English -> Unicode -> Classic -> English.
[[nodiscard]] constexpr InputMode nextMode(InputMode m) noexcept {
    switch (m) {
    case InputMode::English: return InputMode::Unicode;
    case InputMode::Unicode: return InputMode::Classic;
    case InputMode::Classic: return InputMode::English;
    }
    return InputMode::English;
}

// Converts a persisted/compartment value; anything unknown falls back to English.
[[nodiscard]] InputMode modeFromInt(std::int32_t value) noexcept;

// Short status label (UTF-8) for the status bar / indicator.
[[nodiscard]] std::u8string_view modeLabel(InputMode m) noexcept;

} // namespace july
