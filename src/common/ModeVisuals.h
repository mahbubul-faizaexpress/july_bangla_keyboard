#pragma once

#include <windows.h>

#include "july/engine/InputMode.h"

namespace july {

struct ModeVisual {
    const wchar_t* label;      // status bar text
    const wchar_t* iconText;   // 1-2 characters for the tray icon
    const wchar_t* tooltip;
    COLORREF background;
};

[[nodiscard]] const ModeVisual& visualFor(InputMode mode) noexcept;

// Renders the tray icon for a mode at the given DPI. Caller owns the HICON (DestroyIcon).
[[nodiscard]] HICON createModeIcon(InputMode mode, UINT dpi) noexcept;

// UI font: Nirmala UI (ships with Windows, covers Bangla), `pixelHeight` tall.
[[nodiscard]] HFONT createUiFont(int pixelHeight, int weight) noexcept;

} // namespace july
