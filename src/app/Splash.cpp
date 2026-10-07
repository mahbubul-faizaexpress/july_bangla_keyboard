#include "Splash.h"

#include <dwmapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>

#include "ModeVisuals.h"
#include "Resource.h"
#include "july/engine/Version.h"

using Microsoft::WRL::ComPtr;

namespace july::app {

namespace {

constexpr wchar_t kClassName[] = L"JulyBanglaSplash";
constexpr UINT_PTR kCloseTimer = 1;
constexpr UINT kVisibleMs = 4500;  // one-shot; the only timer in the program
constexpr int kWidthDip = 720;
constexpr int kArtHeightDip = 405;  // 16:9, the artwork's aspect ratio
constexpr int kBandHeightDip = 160;

constexpr COLORREF kRed = RGB(0xF4, 0x2A, 0x41);
constexpr COLORREF kGreen = RGB(0x00, 0x6A, 0x4E);

constexpr COLORREF kCream = RGB(0xFF, 0xF4, 0xE2);
constexpr COLORREF kSoft = RGB(0xB8, 0xC4, 0xBE);
constexpr COLORREF kFaint = RGB(0x7E, 0x8C, 0x86);

int dip(int value, UINT dpi) noexcept { return MulDiv(value, static_cast<int>(dpi), 96); }

void drawText(HDC dc, const wchar_t* text, RECT rc, int pixelHeight, int weight, COLORREF color, UINT format) noexcept {
    HFONT font = createUiFont(pixelHeight, weight);
    HGDIOBJ old = SelectObject(dc, font);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &rc, format | DT_NOPREFIX);
    SelectObject(dc, old);
    DeleteObject(font);
}

// Decodes the embedded artwork with WIC into a 32-bit DIB. COM is initialized by the
// application before any splash is shown.
HBITMAP loadArtwork(HINSTANCE instance, int& width, int& height) noexcept {
    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_SPLASH_IMAGE), RT_RCDATA);
    HGLOBAL loaded = resource ? LoadResource(instance, resource) : nullptr;
    const void* bytes = loaded ? LockResource(loaded) : nullptr;
    const DWORD size = resource ? SizeofResource(instance, resource) : 0;
    if (bytes == nullptr || size == 0) return nullptr;

    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(hr)) hr = factory->CreateStream(&stream);
    if (SUCCEEDED(hr)) hr = stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(bytes)), size);
    if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                   WICBitmapPaletteTypeCustom);
    }
    UINT w = 0;
    UINT h = 0;
    if (SUCCEEDED(hr)) hr = converter->GetSize(&w, &h);
    if (FAILED(hr) || w == 0 || h == 0) return nullptr;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = static_cast<LONG>(w);
    info.bmiHeader.biHeight = -static_cast<LONG>(h);  // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap == nullptr) return nullptr;
    const UINT stride = w * 4;
    if (FAILED(converter->CopyPixels(nullptr, stride, stride * h, static_cast<BYTE*>(pixels)))) {
        DeleteObject(bitmap);
        return nullptr;
    }
    width = static_cast<int>(w);
    height = static_cast<int>(h);
    return bitmap;
}

} // namespace

void Splash::show(HINSTANCE instance, bool quitOnClose) noexcept {
    quitOnClose_ = quitOnClose;
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
    artwork_ = loadArtwork(instance, artworkWidth_, artworkHeight_);
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClassName, L"July Bangla Keyboard", WS_POPUP, 0, 0, 1,
                            1, nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) {
        if (artwork_ != nullptr) DeleteObject(artwork_);
        artwork_ = nullptr;
        return;
    }
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
    MONITORINFO info{};
    info.cbSize = sizeof info;
    GetMonitorInfoW(MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &info);
    SetWindowPos(hwnd_, nullptr, info.rcWork.left, info.rcWork.top, 1, 1, SWP_NOZORDER | SWP_NOACTIVATE);
    const UINT dpi = GetDpiForWindow(hwnd_);
    const int width = dip(kWidthDip, dpi);
    const int height = dip(kArtHeightDip + kBandHeightDip, dpi);
    const int x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2;
    const int y = info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - height) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void Splash::paintFallbackHeader(HDC dc, const RECT& area, UINT dpi) noexcept {
    // Used only if the artwork cannot be decoded: the July red band with the name.
    HBRUSH red = CreateSolidBrush(kRed);
    FillRect(dc, &area, red);
    DeleteObject(red);
    RECT name = area;
    name.bottom = area.top + (area.bottom - area.top) * 2 / 3;
    drawText(dc, L"জুলাই", name, dip(96, dpi), FW_BOLD, RGB(0xFF, 0xFF, 0xFF), DT_CENTER | DT_SINGLELINE | DT_BOTTOM);
    RECT subtitle{area.left, name.bottom, area.right, area.bottom - dip(24, dpi)};
    drawText(dc, L"বাংলা কীবোর্ড", subtitle, dip(30, dpi), FW_SEMIBOLD, RGB(0xFF, 0xFF, 0xFF),
             DT_CENTER | DT_SINGLELINE | DT_TOP);
}

void Splash::paint() noexcept {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    const UINT dpi = GetDpiForWindow(hwnd_);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const RECT art{client.left, client.top, client.right, client.top + dip(kArtHeightDip, dpi)};

    if (artwork_ != nullptr) {
        HDC memory = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(memory, artwork_);
        SetStretchBltMode(dc, HALFTONE);
        SetBrushOrgEx(dc, 0, 0, nullptr);
        StretchBlt(dc, art.left, art.top, art.right - art.left, art.bottom - art.top, memory, 0, 0, artworkWidth_,
                   artworkHeight_, SRCCOPY);
        SelectObject(memory, old);
        DeleteDC(memory);
    } else {
        paintFallbackHeader(dc, art, dpi);
    }

    // Band below the artwork: a thin red-and-green ribbon, then the slogan.
    RECT redLine{client.left, art.bottom, client.right, art.bottom + dip(3, dpi)};
    HBRUSH red = CreateSolidBrush(kRed);
    FillRect(dc, &redLine, red);
    DeleteObject(red);
    RECT greenLine{client.left, redLine.bottom, client.right, redLine.bottom + dip(3, dpi)};
    HBRUSH green = CreateSolidBrush(kGreen);
    FillRect(dc, &greenLine, green);
    DeleteObject(green);
    RECT band{client.left, greenLine.bottom, client.right, client.bottom};
    // Soft vertical gradient, from the artwork's dark green to near-black.
    TRIVERTEX vertices[2] = {
        {band.left, band.top, 0x1A00, 0x3400, 0x2C00, 0xFF00},
        {band.right, band.bottom, 0x0B00, 0x1600, 0x1300, 0xFF00},
    };
    GRADIENT_RECT gradient{0, 1};
    GradientFill(dc, vertices, 2, &gradient, 1, GRADIENT_FILL_RECT_V);

    SetBkMode(dc, TRANSPARENT);
    const int centerX = (band.left + band.right) / 2;

    // Slogan: two balanced lines, the most prominent text.
    RECT slogan{band.left + dip(24, dpi), band.top + dip(14, dpi), band.right - dip(24, dpi), band.top + dip(78, dpi)};
    drawText(dc, kSlogan, slogan, dip(24, dpi), FW_SEMIBOLD, kCream, DT_CENTER | DT_WORDBREAK);

    // Ornament: a small red sun between two thin lines.
    const int ornamentY = band.top + dip(88, dpi);
    const int lineHalf = dip(64, dpi);
    const int gap = dip(10, dpi);
    HPEN pen = CreatePen(PS_SOLID, std::max(1, dip(1, dpi)), RGB(0x5E, 0x75, 0x6C));
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, centerX - gap - lineHalf, ornamentY, nullptr);
    LineTo(dc, centerX - gap, ornamentY);
    MoveToEx(dc, centerX + gap, ornamentY, nullptr);
    LineTo(dc, centerX + gap + lineHalf, ornamentY);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
    const int sun = dip(4, dpi);
    HBRUSH sunBrush = CreateSolidBrush(kRed);
    HGDIOBJ oldBrush = SelectObject(dc, sunBrush);
    HGDIOBJ noPen = SelectObject(dc, GetStockObject(NULL_PEN));
    Ellipse(dc, centerX - sun, ornamentY - sun, centerX + sun + 1, ornamentY + sun + 1);
    SelectObject(dc, noPen);
    SelectObject(dc, oldBrush);
    DeleteObject(sunBrush);

    // Dedication.
    RECT dedication{band.left + dip(24, dpi), ornamentY + dip(6, dpi), band.right - dip(24, dpi),
                    ornamentY + dip(30, dpi)};
    drawText(dc, kDedication, dedication, dip(15, dpi), FW_NORMAL, kSoft, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    // Footer: name and version in Bangla digits, and the shortcut.
    wchar_t version[24] = {};
    {
        const char* v = kAppVersion;
        int i = 0;
        for (; v[i] != '\0' && i < 23; ++i) {
            version[i] = (v[i] >= '0' && v[i] <= '9') ? static_cast<wchar_t>(0x09E6 + (v[i] - '0')) : static_cast<wchar_t>(v[i]);
        }
        version[i] = L'\0';
    }
    wchar_t footer[128];
    swprintf_s(footer, L"জুলাই বাংলা কীবোর্ড   ·   সংস্করণ %s   ·   Ctrl+Alt+B", version);
    RECT foot{band.left, band.bottom - dip(26, dpi), band.right, band.bottom - dip(8, dpi)};
    drawText(dc, footer, foot, dip(12, dpi), FW_NORMAL, kFaint, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

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
    case WM_ERASEBKGND:
        return 1;  // everything is painted in WM_PAINT (no flicker)
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
        if (artwork_ != nullptr) DeleteObject(artwork_);  // free the image memory again
        artwork_ = nullptr;
        if (quitOnClose_) PostQuitMessage(0);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    default:
        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }
}

} // namespace july::app
