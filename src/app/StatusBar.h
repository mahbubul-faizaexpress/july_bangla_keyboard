#pragma once

#include <windows.h>

#include "july/engine/InputMode.h"

namespace july::app {

// Small borderless, always-on-top mode indicator (like the Bijoy bar). It never takes the
// keyboard focus away from the application being typed in. Click = next mode, drag =
// move, right-click = the tray menu. Created only when enabled; repaints only when the
// mode, DPI or opacity changes.
class StatusBar {
public:
    struct Callbacks {
        void* context = nullptr;
        void (*onClick)(void* context) = nullptr;
        void (*onContextMenu)(void* context, POINT screen) = nullptr;
        void (*onMoved)(void* context, int x, int y) = nullptr;
    };

    bool create(HINSTANCE instance, const Callbacks& callbacks, InputMode mode, int x, int y, int opacityPercent) noexcept;
    void destroy() noexcept;
    [[nodiscard]] bool exists() const noexcept { return hwnd_ != nullptr; }

    void setMode(InputMode mode) noexcept;
    void setOpacity(int percent) noexcept;

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT handle(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    void layout(UINT dpi, const RECT* suggested) noexcept;
    void paint() noexcept;
    void placeDefault() noexcept;

    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;
    Callbacks callbacks_;
    InputMode mode_ = InputMode::English;
    bool tracking_ = false;
    bool dragging_ = false;
    POINT pressPoint_{};
    POINT pressWindow_{};
};

} // namespace july::app
