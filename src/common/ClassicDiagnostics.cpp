#include "ClassicDiagnostics.h"

#include <windows.h>

namespace july {

namespace {

int CALLBACK onFontFamily(const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM found) {
    *reinterpret_cast<bool*>(found) = true;
    return 0;  // stop at the first match
}

bool fontFamilyInstalled(const wchar_t* family) noexcept {
    HDC dc = GetDC(nullptr);
    if (dc == nullptr) return false;
    LOGFONTW query{};
    query.lfCharSet = DEFAULT_CHARSET;
    if (lstrcpynW(query.lfFaceName, family, LF_FACESIZE) == nullptr) {
        ReleaseDC(nullptr, dc);
        return false;
    }
    bool found = false;
    EnumFontFamiliesExW(dc, &query, onFontFamily, reinterpret_cast<LPARAM>(&found), 0);
    ReleaseDC(nullptr, dc);
    return found;
}

} // namespace

ClassicDiagnostics checkClassicEnvironment() noexcept {
    ClassicDiagnostics d;
    d.sutonnyInstalled = fontFamilyInstalled(L"SutonnyMJ");
    d.ansiCodePage = GetACP();
    d.ansiCodePageOk = d.ansiCodePage == 1252;
    return d;
}

} // namespace july
