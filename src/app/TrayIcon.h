#pragma once

#include <windows.h>
#include <shellapi.h>

#include "july/engine/InputMode.h"

namespace july::app {

// Notification-area icon showing the current mode. Left click = next mode, right click =
// menu. Re-adds itself when Explorer restarts (TaskbarCreated).
class TrayIcon {
public:
    static constexpr UINT kCallbackMessage = WM_APP + 1;

    bool add(HWND owner, InputMode mode) noexcept;
    void update(InputMode mode) noexcept;
    void remove() noexcept;

    // Call from the owner's window procedure; returns true if the message was a
    // TaskbarCreated broadcast (icon re-added).
    bool handleTaskbarCreated(UINT msg) noexcept;
    // DPI of the owner changed: re-render the icon.
    void refresh() noexcept { update(mode_); }

private:
    HWND owner_ = nullptr;
    HICON icon_ = nullptr;
    InputMode mode_ = InputMode::English;
    UINT taskbarCreated_ = 0;
    bool added_ = false;
};

} // namespace july::app
