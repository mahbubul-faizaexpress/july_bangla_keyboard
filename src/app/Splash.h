#pragma once

#include <windows.h>

namespace july::app {

// Remembrance splash shown when the user opens the program (not at Windows startup):
// the July artwork on top, the slogan and dedication on a band below it. It fades in
// without taking the keyboard focus, stays 5 seconds (a click closes it, but only after
// 3.5 seconds so it can be read), and fades out. The artwork is decoded (WIC) only while
// the splash is visible and released when it closes.
class Splash {
public:
    // quitOnClose: post WM_QUIT when the splash closes (standalone preview mode).
    void show(HINSTANCE instance, bool quitOnClose = false) noexcept;
    void close() noexcept;

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT handle(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    void layout() noexcept;
    void paint(HDC dc) noexcept;
    void dismiss() noexcept;  // fade out, then close
    void paintFallbackHeader(HDC dc, const RECT& area, UINT dpi) noexcept;

    HWND hwnd_ = nullptr;
    bool quitOnClose_ = false;
    ULONGLONG shownAt_ = 0;
    HBITMAP artwork_ = nullptr;  // premultiplied 32-bit DIB of the artwork, if decoded
    int artworkWidth_ = 0;
    int artworkHeight_ = 0;
};

// Shared wording (also used by the About box).
// Two balanced lines: the break after the comma is intentional.
inline constexpr wchar_t kSlogan[] = L"জুলাইয়ের অনুভূতি স্মরণ হোক,\nবাংলা লেখার মাধ্যমে।";
inline constexpr wchar_t kDedication[] = L"২০২৪ সালের জুলাইয়ের ছাত্র-জনতার গণঅভ্যুত্থানের স্মরণে";

} // namespace july::app
