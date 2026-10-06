// JulyBangla.exe: companion process for the July Bangla Keyboard text service.
//
// Shows the input mode (tray icon + floating status bar), lets the user switch it, and
// remembers settings per user. It is not on the typing path: typing happens inside the
// text service in each application. Event-driven only (GetMessage loop, no timers).

#include <windows.h>
#include <windowsx.h>

#include <cstdio>

#include "ClassicDiagnostics.h"
#include "ModeSync.h"
#include "ModeVisuals.h"
#include "Settings.h"
#include "StatusBar.h"
#include "TrayIcon.h"
#include "july/engine/Version.h"

namespace july::app {

namespace {

constexpr wchar_t kHiddenClass[] = L"JulyBanglaCompanion";
constexpr wchar_t kMutexName[] = L"Local\\JulyBanglaKeyboard.Companion";
constexpr UINT kShowBarMessage = WM_APP + 2;  // sent by a second instance

enum Command : UINT {
    kCmdEnglish = 101,
    kCmdUnicode,
    kCmdClassic,
    kCmdShowBar = 110,
    kCmdOpacity100 = 120,
    kCmdOpacity90,
    kCmdOpacity75,
    kCmdOpacity60,
    kCmdStartup = 130,
    kCmdDiagnostics = 140,
    kCmdAbout,
    kCmdExit = 150,
};

constexpr int kOpacityChoices[] = {100, 90, 75, 60};

class App {
public:
    int run(HINSTANCE instance) noexcept;

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

    static void onModeChanged(void* self, InputMode mode) noexcept;
    static void onBarClick(void* self) noexcept;
    static void onBarMenu(void* self, POINT pt) noexcept;
    static void onBarMoved(void* self, int x, int y) noexcept;

    void cycleMode() noexcept;
    void setMode(InputMode mode) noexcept;
    void showMenu(POINT pt) noexcept;
    void command(UINT id) noexcept;
    void setStatusBarVisible(bool visible) noexcept;
    void showDiagnostics() noexcept;
    void showAbout() noexcept;

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    Settings settings_;
    ModeSync modeSync_;
    TrayIcon tray_;
    StatusBar bar_;
};

int App::run(HINSTANCE instance) noexcept {
    instance_ = instance;
    settings_ = loadSettings();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = &App::windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kHiddenClass;
    if (RegisterClassExW(&wc) == 0) return 1;
    // Hidden top-level window: owns the tray icon and receives TaskbarCreated broadcasts.
    hwnd_ = CreateWindowExW(0, kHiddenClass, L"July Bangla Keyboard", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr,
                            instance, this);
    if (hwnd_ == nullptr) return 1;

    modeSync_.start(&App::onModeChanged, this, settings_.mode);
    const InputMode mode = modeSync_.mode();
    tray_.add(hwnd_, mode);
    if (settings_.showStatusBar) setStatusBarVisible(true);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    tray_.remove();
    bar_.destroy();
    modeSync_.stop();
    return 0;
}

void App::onModeChanged(void* self, InputMode mode) noexcept {
    auto* app = static_cast<App*>(self);
    app->tray_.update(mode);
    app->bar_.setMode(mode);
    saveMode(mode);  // restored at next start
}

void App::onBarClick(void* self) noexcept { static_cast<App*>(self)->cycleMode(); }
void App::onBarMenu(void* self, POINT pt) noexcept { static_cast<App*>(self)->showMenu(pt); }

void App::onBarMoved(void* self, int x, int y) noexcept {
    auto* app = static_cast<App*>(self);
    app->settings_.statusBarX = x;
    app->settings_.statusBarY = y;
    saveStatusBarPosition(x, y);
}

void App::cycleMode() noexcept { setMode(nextMode(modeSync_.mode())); }

void App::setMode(InputMode mode) noexcept {
    // The change comes back through ModeSync::OnChange, which updates the UI; that keeps
    // this process and every application on the same path.
    if (FAILED(modeSync_.setMode(mode))) onModeChanged(this, mode);
}

void App::setStatusBarVisible(bool visible) noexcept {
    settings_.showStatusBar = visible;
    saveStatusBarVisible(visible);
    if (!visible) {
        bar_.destroy();
        return;
    }
    StatusBar::Callbacks callbacks;
    callbacks.context = this;
    callbacks.onClick = &App::onBarClick;
    callbacks.onContextMenu = &App::onBarMenu;
    callbacks.onMoved = &App::onBarMoved;
    bar_.create(instance_, callbacks, modeSync_.mode(), settings_.statusBarX, settings_.statusBarY,
                settings_.opacityPercent);
}

void App::showMenu(POINT pt) noexcept {
    HMENU menu = CreatePopupMenu();
    HMENU opacity = CreatePopupMenu();
    if (menu == nullptr || opacity == nullptr) return;

    const InputMode mode = modeSync_.mode();
    AppendMenuW(menu, MF_STRING, kCmdEnglish, L"English\tCtrl+Alt+B");
    AppendMenuW(menu, MF_STRING, kCmdUnicode, L"বাংলা (Unicode)");
    AppendMenuW(menu, MF_STRING, kCmdClassic, L"বিজয় (Classic / SutonnyMJ)");
    const UINT checkedMode = mode == InputMode::Unicode ? kCmdUnicode : mode == InputMode::Classic ? kCmdClassic : kCmdEnglish;
    CheckMenuRadioItem(menu, kCmdEnglish, kCmdClassic, checkedMode, MF_BYCOMMAND);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(menu, MF_STRING | (settings_.showStatusBar ? MF_CHECKED : 0), kCmdShowBar,
                L"স্ট্যাটাস বার দেখাও (Status bar)");
    for (int i = 0; i < 4; ++i) {
        wchar_t label[16];
        swprintf_s(label, L"%d%%", kOpacityChoices[i]);
        AppendMenuW(opacity, MF_STRING | (settings_.opacityPercent == kOpacityChoices[i] ? MF_CHECKED : 0),
                    kCmdOpacity100 + static_cast<UINT>(i), label);
    }
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(opacity), L"স্বচ্ছতা (Opacity)");
    AppendMenuW(menu, MF_STRING | (startsWithWindows() ? MF_CHECKED : 0), kCmdStartup,
                L"Windows চালু হলে চালু (Start with Windows)");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCmdDiagnostics, L"Classic ডায়াগনস্টিক (Diagnostics)");
    AppendMenuW(menu, MF_STRING, kCmdAbout, L"সম্পর্কে (About)");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCmdExit, L"বন্ধ করো (Exit)");

    // Documented requirement for notification-area menus: foreground first, WM_NULL after.
    SetForegroundWindow(hwnd_);
    const UINT id = static_cast<UINT>(TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x,
                                                       pt.y, hwnd_, nullptr));
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(menu);  // also destroys the opacity submenu
    if (id != 0) command(id);
}

void App::command(UINT id) noexcept {
    switch (id) {
    case kCmdEnglish: setMode(InputMode::English); break;
    case kCmdUnicode: setMode(InputMode::Unicode); break;
    case kCmdClassic: setMode(InputMode::Classic); break;
    case kCmdShowBar: setStatusBarVisible(!settings_.showStatusBar); break;
    case kCmdOpacity100: case kCmdOpacity90: case kCmdOpacity75: case kCmdOpacity60:
        settings_.opacityPercent = kOpacityChoices[id - kCmdOpacity100];
        saveOpacity(settings_.opacityPercent);
        bar_.setOpacity(settings_.opacityPercent);
        break;
    case kCmdStartup: {
        wchar_t path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (length > 0 && length < MAX_PATH) setStartWithWindows(!startsWithWindows(), path);
        break;
    }
    case kCmdDiagnostics: showDiagnostics(); break;
    case kCmdAbout: showAbout(); break;
    case kCmdExit: DestroyWindow(hwnd_); break;
    default: break;
    }
}

void App::showDiagnostics() noexcept {
    const ClassicDiagnostics d = checkClassicEnvironment();
    wchar_t text[768];
    swprintf_s(text,
               L"SutonnyMJ font: %s\n"
               L"ANSI code page: %u %s\n\n"
               L"Classic (বিজয়) mode writes SutonnyMJ codes. Select the SutonnyMJ font in your "
               L"application to see Bangla.%s",
               d.sutonnyInstalled ? L"installed" : L"NOT installed (Classic text will look like English letters)",
               d.ansiCodePage, d.ansiCodePageOk ? L"(OK)" : L"(not 1252: Classic text will not round-trip in non-Unicode programs)",
               d.sutonnyInstalled ? L"" : L"\nThis program never downloads fonts.");
    MessageBoxW(nullptr, text, L"July Bangla Keyboard — Classic diagnostics", MB_OK | MB_ICONINFORMATION);
}

void App::showAbout() noexcept {
    wchar_t text[512];
    swprintf_s(text,
               L"July Bangla Keyboard %hs\nEngine %hs, layout %hs (Bijoy)\n\n"
               L"Ctrl+Alt+B: English → বাংলা → বিজয়\n\n"
               L"Privacy: works offline. No keystrokes, typed text or clipboard data are stored or sent.",
               kAppVersion, kEngineVersion, kLayoutVersion);
    MessageBoxW(nullptr, text, L"July Bangla Keyboard", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK App::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (msg == WM_NCCREATE) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
    }
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return app != nullptr ? app->handle(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    if (tray_.handleTaskbarCreated(msg)) return 0;
    switch (msg) {
    case TrayIcon::kCallbackMessage:
        switch (LOWORD(lParam)) {
        case NIN_SELECT:
        case NIN_KEYSELECT:
            cycleMode();
            return 0;
        case WM_CONTEXTMENU:
            showMenu(POINT{GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)});
            return 0;
        default:
            return 0;
        }
    case kShowBarMessage:
        if (!settings_.showStatusBar) setStatusBarVisible(true);
        return 0;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        tray_.refresh();  // icon size may depend on DPI / scaling
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

} // namespace july::app

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, july::app::kMutexName);
    if (mutex == nullptr) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Already running: ask that instance to show its status bar, then leave.
        if (HWND existing = FindWindowW(july::app::kHiddenClass, nullptr)) {
            PostMessageW(existing, july::app::kShowBarMessage, 0, 0);
        }
        CloseHandle(mutex);
        return 0;
    }

    int result = 1;
    if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        static july::app::App app;  // static: large members, single instance
        result = app.run(instance);
        CoUninitialize();
    }
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return result;
}
