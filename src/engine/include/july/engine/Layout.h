#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "july/engine/Token.h"

namespace july {

// The four quadrants printed on a Bijoy key cap:
//   Normal    = key alone          Shift     = Shift + key
//   Link      = 'g' then key       LinkShift = 'g' then Shift + key
enum class KeyLayer : std::uint8_t { Normal, Shift, Link, LinkShift };
inline constexpr std::size_t kKeyLayerCount = 4;

// Keys are identified by PC/AT set-1 scan code (positional, independent of the active
// Latin layout). Extended-key scan codes are never mapped.
inline constexpr std::size_t kScanCodeCount = 0x80;

// Where a layout row's mapping came from. Release builds require every row to be
// confirmed (anything except Memory).
enum class LayoutSource : std::uint8_t {
    Chart,     // Bijoy keyboard chart, 3rd edition (images supplied by the user)
    UserList,  // conjunct key sequences supplied by the user
    Memory,    // engineer's recollection, NOT yet confirmed against a reference
};

struct LayoutEntry {
    std::uint8_t scan;
    KeyLayer layer;
    LayoutSource source;
    Token token;
};

// Returns the token for a key in a layer; a TokenKind::None token if unmapped.
[[nodiscard]] const Token& lookupBijoyKey(std::uint16_t scan, KeyLayer layer) noexcept;

// All rows of the Bijoy layout, in source-file order (for tests and diagnostics).
[[nodiscard]] std::span<const LayoutEntry> bijoyLayoutEntries() noexcept;

} // namespace july
