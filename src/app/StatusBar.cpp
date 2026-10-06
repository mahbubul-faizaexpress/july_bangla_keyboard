#include "StatusBar.h"

#include <dwmapi.h>

#include <cstdlib>

#include "ModeVisuals.h"
#include "Settings.h"

namespace july::app {

namespace {

constexpr wchar_t kClassName[] = L"JulyBanglaStatusBar";
constexpr int kWidthDip = 104;
constexpr int kHeightDip = 34;
constexpr int kFontDip = 18;
constexpr int kTopMarginDip = 8;

BYTE alphaFor(int percent) noexcept {
    return static_cast<BYTE>(MulDiv(percent < 40 ? 40 : (percent > 100 ? 100 : percent), 255, 100));
}

} // namespace

bool StatusBar::create(HINSTANCE instance, const Callbacks& callbacks, InputMode mode, int x, int y,
                       int opacityPercent) noexcept {
    if (hwnd_ != nullptr) return true;
    callbacks_ = callbacks;
    mode_ = mode;

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof wc;
        wc.lpfnWndProc = &StatusBar::windowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
        wc.lpszClassName = kClassName;
        registered = RegisterClassExW(&wc) != 0;
        if (!registered) return false;
    }

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED, kClassName,
                            L"July Bangla Keyboard", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) return false;

    SetLayeredWindowAttributes(hwnd_, 0, alphaFor(opacityPercent), LWA_ALPHA);
    const DWM_WINDOW_CORNER_PREFERENCE round = DWMWCP_ROUND;  // Windows 11; ignored elsewhere
    DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &round, sizeof round);

    layout(GetDpiForWindow(hwnd_), nullptr);
    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    const RECT wanted{x, y, x + width, y + height};
    if (x == kUnsetPosition || y == kUnsetPosition || MonitorFromRect(&wanted, MONITOR_DEFAULTTONULL) == nullptr) {
        placeDefault();  // first run, or the saved position is on a monitor that is gone
    } else {
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        layout(GetDpiForWindow(hwnd_), nullptr);  // the monitor may have a different DPI
    }
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    return true;
}

void StatusBar::destroy() noexcept {
    if (hwnd_ != nullptr) DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    if (font_ != nullptr) DeleteObject(font_);
    font_ = nullptr;
}

void StatusBar::setMode(InputMode mode) noexcept {
    mode_ = mode;
    if (hwnd_ != nullptr) InvalidateRect(hwnd_, nullptr, FALSE);
}

void StatusBar::setOpacity(int percent) noexcept {
    if (hwnd_ != nullptr) SetLayeredWindowAttributes(hwnd_, 0, alphaFor(percent), LWA_ALPHA);
}

void StatusBar::layout(UINT dpi, const RECT* suggested) noexcept {
    if (font_ != nullptr) DeleteObject(font_);
    font_ = createUiFont(MulDiv(kFontDip, static_cast<int>(dpi), 96), FW_SEMIBOLD);
    const int width = MulDiv(kWidthDip, static_cast<int>(dpi), 96);
    const int height = MulDiv(kHeightDip, static_cast<int>(dpi), 96);
    if (suggested != nullptr) {
        SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        SetWindowPos(hwnd_, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void StatusBar::placeDefault() noexcept {
    // Top centre of the primary monitor's work area, like the Bijoy bar.
    const POINT origin{0, 0};
    MONITORINFO info{};
    info.cbSize = sizeof info;
    GetMonitorInfoW(MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY), &info);
    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    const int width = rc.right - rc.left;
    const int x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2;
    const int y = info.rcWork.top + MulDiv(kTopMarginDip, static_cast<int>(GetDpiForWindow(hwnd_)), 96);
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void StatusBar::paint() noexcept {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const ModeVisual& visual = visualFor(mode_);
    HBRUSH brush = CreateSolidBrush(visual.background);
    FillRect(dc, &rc, brush);
    DeleteObject(brush);
    HGDIOBJ oldFont = SelectObject(dc, font_);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
    DrawTextW(dc, visual.label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK StatusBar::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (msg == WM_NCCREATE) {
        auto* self = static_cast<StatusBar*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<StatusBar*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) return DefWindowProcW(hwnd, msg, wParam, lParam);
    return self->handle(msg, wParam, lParam);
}

LRESULT StatusBar::handle(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    switch (msg) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;  // never steal focus from the application being typed in
    case WM_PAINT:
        paint();
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(hwnd_);
        tracking_ = true;
        dragging_ = false;
        GetCursorPos(&pressPoint_);
        {
            RECT rc{};
            GetWindowRect(hwnd_, &rc);
            pressWindow_ = {rc.left, rc.top};
        }
        return 0;
    case WM_MOUSEMOVE:
        if (tracking_) {
            POINT pt{};
            GetCursorPos(&pt);
            const int dx = pt.x - pressPoint_.x;
            const int dy = pt.y - pressPoint_.y;
            if (!dragging_ && (std::abs(dx) > GetSystemMetrics(SM_CXDRAG) || std::abs(dy) > GetSystemMetrics(SM_CYDRAG))) {
                dragging_ = true;
            }
            if (dragging_) {
                SetWindowPos(hwnd_, nullptr, pressWindow_.x + dx, pressWindow_.y + dy, 0, 0,
                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (tracking_) {
            tracking_ = false;
            ReleaseCapture();
            if (dragging_) {
                RECT rc{};
                GetWindowRect(hwnd_, &rc);
                if (callbacks_.onMoved != nullptr) callbacks_.onMoved(callbacks_.context, rc.left, rc.top);
            } else if (callbacks_.onClick != nullptr) {
                callbacks_.onClick(callbacks_.context);
            }
            dragging_ = false;
        }
        return 0;
    case WM_CAPTURECHANGED:
        tracking_ = false;
        return 0;
    case WM_RBUTTONUP:
        if (callbacks_.onContextMenu != nullptr) {
            POINT pt{};
            GetCursorPos(&pt);
            callbacks_.onContextMenu(callbacks_.context, pt);
        }
        return 0;
    case WM_DPICHANGED:
        layout(HIWORD(wParam), reinterpret_cast<const RECT*>(lParam));
        return 0;
    case WM_NCDESTROY: {
        HWND hwnd = hwnd_;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        hwnd_ = nullptr;
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

} // namespace july::app
