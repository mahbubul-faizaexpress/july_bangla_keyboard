#include "Splash.h"

#include <dwmapi.h>

#include "ModeVisuals.h"
#include "july/engine/Version.h"

namespace july::app {

namespace {

constexpr wchar_t kClassName[] = L"JulyBanglaSplash";
constexpr UINT_PTR kCloseTimer = 1;
constexpr UINT kVisibleMs = 4000;  // one-shot; the only timer in the program
constexpr int kWidthDip = 560;
constexpr int kHeightDip = 320;

constexpr COLORREF kRed = RGB(0xF4, 0x2A, 0x41);
constexpr COLORREF kGreen = RGB(0x00, 0x6A, 0x4E);
constexpr COLORREF kCream = RGB(0xFF, 0xF8, 0xEE);
constexpr COLORREF kInk = RGB(0x2B, 0x2B, 0x2B);
constexpr COLORREF kMuted = RGB(0x6B, 0x6B, 0x6B);

int dip(int value, UINT dpi) noexcept { return MulDiv(value, static_cast<int>(dpi), 96); }

void drawText(HDC dc, const wchar_t* text, RECT rc, int pixelHeight, int weight, COLORREF color, UINT format) noexcept {
    HFONT font = createUiFont(pixelHeight, weight);
    HGDIOBJ old = SelectObject(dc, font);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &rc, format | DT_NOPREFIX);
    SelectObject(dc, old);
    DeleteObject(font);
}

} // namespace

void Splash::show(HINSTANCE instance) noexcept {
    if (hwnd_ != nullptr) {
        SetTimer(hwnd_, kCloseTimer, kVisibleMs, nullptr);  // shown again: restart the countdown
        return;
    }
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof wc;
        wc.lpfnWndProc = &Splash::windowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kClassName;
        registered = RegisterClassExW(&wc) != 0;
        if (!registered) return;
    }
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClassName, L"July Bangla Keyboard", WS_POPUP, 0, 0, 1,
                            1, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) return;
    const DWM_WINDOW_CORNER_PREFERENCE round = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE, &round, sizeof round);
    layout();
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);  // so that a key press can dismiss it
    SetTimer(hwnd_, kCloseTimer, kVisibleMs, nullptr);
}

void Splash::close() noexcept {
    if (hwnd_ != nullptr) DestroyWindow(hwnd_);
}

void Splash::layout() noexcept {
    // Centre on the monitor that has the mouse, sized for that monitor's DPI.
    POINT cursor{};
    GetCursorPos(&cursor);
    HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{};
    info.cbSize = sizeof info;
    GetMonitorInfoW(monitor, &info);
    SetWindowPos(hwnd_, nullptr, info.rcWork.left, info.rcWork.top, 1, 1, SWP_NOZORDER | SWP_NOACTIVATE);
    const UINT dpi = GetDpiForWindow(hwnd_);
    const int width = dip(kWidthDip, dpi);
    const int height = dip(kHeightDip, dpi);
    const int x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2;
    const int y = info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - height) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void Splash::paint() noexcept {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    const UINT dpi = GetDpiForWindow(hwnd_);
    RECT client{};
    GetClientRect(hwnd_, &client);

    HBRUSH cream = CreateSolidBrush(kCream);
    FillRect(dc, &client, cream);
    DeleteObject(cream);

    // Red band on top (the "red profile" of July) with a green ribbon line under it.
    RECT band{client.left, client.top, client.right, client.top + dip(118, dpi)};
    HBRUSH red = CreateSolidBrush(kRed);
    FillRect(dc, &band, red);
    DeleteObject(red);
    RECT ribbon{client.left, band.bottom, client.right, band.bottom + dip(6, dpi)};
    HBRUSH green = CreateSolidBrush(kGreen);
    FillRect(dc, &ribbon, green);
    DeleteObject(green);

    SetBkMode(dc, TRANSPARENT);
    const UINT centered = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

    RECT name = band;
    name.bottom = band.top + dip(84, dpi);
    drawText(dc, L"জুলাই", name, dip(58, dpi), FW_BOLD, RGB(0xFF, 0xFF, 0xFF), centered);
    RECT subtitle{band.left, band.top + dip(78, dpi), band.right, band.bottom - dip(6, dpi)};
    drawText(dc, L"বাংলা কীবোর্ড", subtitle, dip(20, dpi), FW_SEMIBOLD, RGB(0xFF, 0xFF, 0xFF), centered);

    RECT slogan{client.left + dip(24, dpi), ribbon.bottom + dip(24, dpi), client.right - dip(24, dpi),
                ribbon.bottom + dip(104, dpi)};
    drawText(dc, kSlogan, slogan, dip(23, dpi), FW_SEMIBOLD, kGreen, DT_CENTER | DT_WORDBREAK);

    RECT dedication{client.left + dip(24, dpi), client.bottom - dip(78, dpi), client.right - dip(24, dpi),
                    client.bottom - dip(48, dpi)};
    drawText(dc, kDedication, dedication, dip(16, dpi), FW_NORMAL, kInk, centered);

    wchar_t footer[96];
    swprintf_s(footer, L"July Bangla Keyboard %hs  ·  Ctrl+Alt+B", kAppVersion);
    RECT foot{client.left, client.bottom - dip(40, dpi), client.right, client.bottom - dip(14, dpi)};
    drawText(dc, footer, foot, dip(13, dpi), FW_NORMAL, kMuted, centered);

    EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK Splash::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (msg == WM_NCCREATE) {
        auto* self = static_cast<Splash*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<Splash*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return self != nullptr ? self->handle(msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Splash::handle(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    switch (msg) {
    case WM_PAINT:
        paint();
        return 0;
    case WM_TIMER:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_KEYDOWN:
        close();
        return 0;
    case WM_DPICHANGED:
        layout();
        InvalidateRect(hwnd_, nullptr, TRUE);
        return 0;
    case WM_NCDESTROY: {
        HWND hwnd = hwnd_;
        KillTimer(hwnd, kCloseTimer);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        hwnd_ = nullptr;
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    default:
        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }
}

} // namespace july::app
