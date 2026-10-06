#pragma once

#include "july/engine/InputMode.h"

// Per-user settings in HKCU\Software\JulyBangla (cold path only; never read while typing).
// Never stores typed text or keystrokes.

namespace july {

inline constexpr int kUnsetPosition = -32768;

struct Settings {
    InputMode mode = InputMode::English;  // last mode, restored at startup
    bool showStatusBar = true;
    int statusBarX = kUnsetPosition;      // physical pixels; unset = default position
    int statusBarY = kUnsetPosition;
    int opacityPercent = 92;              // 40..100
};

[[nodiscard]] Settings loadSettings() noexcept;
void saveMode(InputMode mode) noexcept;
void saveStatusBarVisible(bool visible) noexcept;
void saveStatusBarPosition(int x, int y) noexcept;
void saveOpacity(int percent) noexcept;

// "Start with Windows": HKCU\Software\Microsoft\Windows\CurrentVersion\Run (per user,
// visible in Task Manager > Startup apps).
[[nodiscard]] bool startsWithWindows() noexcept;
bool setStartWithWindows(bool enable, const wchar_t* exePath) noexcept;

} // namespace july
