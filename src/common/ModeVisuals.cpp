#include "ModeVisuals.h"

namespace july {

namespace {

// Bangla strings as escapes so the exact code points are unambiguous.
const ModeVisual kVisuals[] = {
    {L"EN", L"EN", L"July Bangla Keyboard — English", RGB(0x55, 0x5F, 0x6D)},
    {L"বাংলা", L"বা",                          // বাংলা / বা
     L"July Bangla Keyboard — বাংলা (Unicode)", RGB(0x1B, 0x7F, 0x3B)},
    // Classic output is SutonnyMJ / Bijoy-compatible; the product does not use the
    // registered "Bijoy" name for its own mode.
    {L"ক্লাসিক", L"ক",                          // ক্লাসিক / ক
     L"July Bangla Keyboard — ক্লাসিক (SutonnyMJ / ANSI)", RGB(0xC0, 0x39, 0x2B)},
};

} // namespace

const ModeVisual& visualFor(InputMode mode) noexcept {
    switch (mode) {
    case InputMode::Unicode: return kVisuals[1];
    case InputMode::Classic: return kVisuals[2];
    case InputMode::English: break;
    }
    return kVisuals[0];
}

HFONT createUiFont(int pixelHeight, int weight) noexcept {
    LOGFONTW lf{};
    lf.lfHeight = -pixelHeight;
    lf.lfWeight = weight;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    (void)lstrcpynW(lf.lfFaceName, L"Nirmala UI", LF_FACESIZE);  // constant, always fits
    return CreateFontIndirectW(&lf);
}

HICON createModeIcon(InputMode mode, UINT dpi) noexcept {
    const int size = GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const ModeVisual& visual = visualFor(mode);

    BITMAPV5HEADER header{};
    header.bV5Size = sizeof header;
    header.bV5Width = size;
    header.bV5Height = -size;  // top-down
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    HDC screen = GetDC(nullptr);
    void* bits = nullptr;
    HBITMAP color = CreateDIBSection(screen, reinterpret_cast<BITMAPINFO*>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC dc = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (color == nullptr || dc == nullptr || bits == nullptr) {
        if (color) DeleteObject(color);
        if (dc) DeleteDC(dc);
        return nullptr;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, color);

    // Rounded square in the mode colour, text in white.
    HBRUSH brush = CreateSolidBrush(visual.background);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
    const int radius = size / 4;
    RoundRect(dc, 0, 0, size + 1, size + 1, radius, radius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(brush);

    HFONT font = createUiFont(MulDiv(size, mode == InputMode::English ? 55 : 75, 100), FW_BOLD);
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
    RECT rc{0, 0, size, size};
    DrawTextW(dc, visual.iconText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    SelectObject(dc, oldBitmap);
    DeleteDC(dc);
    GdiFlush();

    // GDI leaves alpha at 0: make painted pixels opaque, untouched corners transparent.
    auto* pixels = static_cast<DWORD*>(bits);
    for (int i = 0; i < size * size; ++i) {
        if ((pixels[i] & 0x00FFFFFF) != 0) pixels[i] |= 0xFF000000;
    }

    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO info{};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = mask;
    HICON icon = CreateIconIndirect(&info);
    DeleteObject(mask);
    DeleteObject(color);
    return icon;
}

} // namespace july
