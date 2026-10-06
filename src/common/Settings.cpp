#include "Settings.h"

#include <windows.h>

#include <algorithm>

namespace july {

namespace {

constexpr wchar_t kKey[] = L"Software\\JulyBangla";
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"JulyBanglaKeyboard";

bool readDword(const wchar_t* name, DWORD& value) noexcept {
    DWORD size = sizeof value;
    return RegGetValueW(HKEY_CURRENT_USER, kKey, name, RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS;
}

void writeDword(const wchar_t* name, DWORD value) noexcept {
    RegSetKeyValueW(HKEY_CURRENT_USER, kKey, name, REG_DWORD, &value, sizeof value);
}

} // namespace

Settings loadSettings() noexcept {
    Settings s;
    DWORD v = 0;
    if (readDword(L"Mode", v)) s.mode = modeFromInt(static_cast<std::int32_t>(v));
    if (readDword(L"ShowStatusBar", v)) s.showStatusBar = v != 0;
    if (readDword(L"StatusBarX", v)) s.statusBarX = static_cast<int>(v);
    if (readDword(L"StatusBarY", v)) s.statusBarY = static_cast<int>(v);
    if (readDword(L"OpacityPercent", v)) s.opacityPercent = std::clamp(static_cast<int>(v), 40, 100);
    return s;
}

void saveMode(InputMode mode) noexcept { writeDword(L"Mode", static_cast<DWORD>(mode)); }
void saveStatusBarVisible(bool visible) noexcept { writeDword(L"ShowStatusBar", visible ? 1 : 0); }
void saveOpacity(int percent) noexcept { writeDword(L"OpacityPercent", static_cast<DWORD>(std::clamp(percent, 40, 100))); }

void saveStatusBarPosition(int x, int y) noexcept {
    writeDword(L"StatusBarX", static_cast<DWORD>(x));
    writeDword(L"StatusBarY", static_cast<DWORD>(y));
}

bool startsWithWindows() noexcept {
    DWORD size = 0;
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, nullptr, &size) == ERROR_SUCCESS;
}

bool setStartWithWindows(bool enable, const wchar_t* exePath) noexcept {
    if (!enable) {
        const LSTATUS status = RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
        return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
    }
    wchar_t command[MAX_PATH + 3] = {};
    if (swprintf_s(command, L"\"%s\"", exePath) < 0) return false;
    const auto bytes = static_cast<DWORD>((wcslen(command) + 1) * sizeof(wchar_t));
    return RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, command, bytes) == ERROR_SUCCESS;
}

} // namespace july
