#pragma once

#include <cstdint>
#include <type_traits>

namespace july {

enum Modifier : std::uint8_t {
    ModShift = 1u << 0,
    ModCtrl  = 1u << 1,
    ModAlt   = 1u << 2,
    ModWin   = 1u << 3,
    ModCaps  = 1u << 4,
};

enum class KeyKind : std::uint8_t { Down, Up };

// Hot-path input event. Platform-neutral: the TIP fills it from wParam/lParam.
struct KeyEvent {
    std::uint16_t vk;    // virtual-key code
    std::uint16_t scan;  // scan code; bit 8 = extended
    std::uint8_t  mods;  // Modifier bitmask
    KeyKind       kind;
    std::uint16_t reserved;
};

static_assert(sizeof(KeyEvent) == 8);
static_assert(std::is_trivially_copyable_v<KeyEvent>);

} // namespace july
