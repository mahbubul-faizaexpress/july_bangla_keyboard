#include "TrayIcon.h"

#include "ModeVisuals.h"

namespace july::app {

namespace {

constexpr UINT kIconId = 1;

NOTIFYICONDATAW baseData(HWND owner) noexcept {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof data;
    data.hWnd = owner;
    data.uID = kIconId;
    return data;
}

} // namespace

bool TrayIcon::add(HWND owner, InputMode mode) noexcept {
    owner_ = owner;
    mode_ = mode;
    if (taskbarCreated_ == 0) taskbarCreated_ = RegisterWindowMessageW(L"TaskbarCreated");

    HICON icon = createModeIcon(mode, GetDpiForWindow(owner));
    NOTIFYICONDATAW data = baseData(owner);
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = kCallbackMessage;
    data.hIcon = icon;
    lstrcpynW(data.szTip, visualFor(mode).tooltip, ARRAYSIZE(data.szTip));
    added_ = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    if (added_) {
        data.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &data);
    }
    if (icon_ != nullptr) DestroyIcon(icon_);
    icon_ = icon;
    return added_;
}

void TrayIcon::update(InputMode mode) noexcept {
    mode_ = mode;
    if (!added_) return;
    HICON icon = createModeIcon(mode, GetDpiForWindow(owner_));
    NOTIFYICONDATAW data = baseData(owner_);
    data.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.hIcon = icon;
    lstrcpynW(data.szTip, visualFor(mode).tooltip, ARRAYSIZE(data.szTip));
    Shell_NotifyIconW(NIM_MODIFY, &data);
    if (icon_ != nullptr) DestroyIcon(icon_);
    icon_ = icon;
}

void TrayIcon::remove() noexcept {
    if (added_) {
        NOTIFYICONDATAW data = baseData(owner_);
        Shell_NotifyIconW(NIM_DELETE, &data);
    }
    added_ = false;
    if (icon_ != nullptr) DestroyIcon(icon_);
    icon_ = nullptr;
}

bool TrayIcon::handleTaskbarCreated(UINT msg) noexcept {
    if (taskbarCreated_ == 0 || msg != taskbarCreated_) return false;
    added_ = false;  // Explorer restarted: the old icon is gone
    add(owner_, mode_);
    return true;
}

} // namespace july::app
