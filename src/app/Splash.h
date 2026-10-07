#pragma once

#include <windows.h>

namespace july::app {

// Remembrance splash shown when the user opens the program (not at Windows startup):
// the July artwork on top, the slogan and dedication on a band below it. Closes after a
// few seconds, on click or on any key. The artwork is decoded (WIC) only while the
// splash is visible and released when it closes.
class Splash {
public:
    void show(HINSTANCE instance) noexcept;
    void close() noexcept;

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT handle(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    void layout() noexcept;
    void paint() noexcept;
    void paintFallbackHeader(HDC dc, const RECT& area, UINT dpi) noexcept;

    HWND hwnd_ = nullptr;
    HBITMAP artwork_ = nullptr;  // premultiplied 32-bit DIB of the artwork, if decoded
    int artworkWidth_ = 0;
    int artworkHeight_ = 0;
};

// Shared wording (also used by the About box).
// Two balanced lines: the break after the comma is intentional.
inline constexpr wchar_t kSlogan[] = L"জুলাইয়ের অনুভূতি স্মরণ হোক,\nবাংলা লেখার মাধ্যমে।";
inline constexpr wchar_t kDedication[] = L"২০২৪ সালের জুলাইয়ের ছাত্র-জনতার গণঅভ্যুত্থানের স্মরণে";

} // namespace july::app
