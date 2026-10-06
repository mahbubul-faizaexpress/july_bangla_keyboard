// july_tip_smoke: end-to-end test of the REGISTERED text service through real TSF.
//
// Creates a RichEdit control with TSF enabled, activates the July Bangla Keyboard profile
// for this process only, sets the input mode through the global compartment, feeds keys
// through ITfKeystrokeMgr (the documented path TSF-aware applications use to deliver keys
// to text services), and compares the control's text with the expected text.
//
// Requires the text service to be registered (tools/dev/Register-Dev.ps1). It does not
// change the user's input settings: the profile is activated for this process only.
//
//   july_tip_smoke.exe        exit 0 = all cases passed

#include <windows.h>
#include <ctffunc.h>
#include <msctf.h>
#include <richedit.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>
#include <string_view>

using Microsoft::WRL::ComPtr;

namespace {

// Must match src/tip/Guids.h.
constexpr GUID kClsidTextService = {0xAF6ABBB0, 0xA4E4, 0x4624, {0xB2, 0xF2, 0x08, 0x85, 0xAA, 0xAB, 0x8A, 0x7F}};
constexpr GUID kGuidProfile = {0xCDFAB3B7, 0x9E90, 0x40DD, {0xB0, 0x4A, 0x19, 0x72, 0x23, 0xA2, 0x65, 0x28}};
constexpr GUID kGuidModeCompartment = {0x540A2534, 0xDA60, 0x41E0, {0xA9, 0x2C, 0xF5, 0x2B, 0x10, 0x13, 0x9F, 0x44}};
constexpr LANGID kLangId = 0x0845;

constexpr LONG kModeEnglish = 0;
constexpr LONG kModeUnicode = 1;
constexpr LONG kModeClassic = 2;

struct Case {
    LONG mode;
    const char* keys;  // letter labels; uppercase = Shift; '<' Backspace, newline Enter, ' ' Space
    const wchar_t* expected;
    const char* name;
};

const Case kCases[] = {
    {kModeUnicode, "gfdm", L"আমি", "আমি"},
    {kModeUnicode, "jgN", L"ক্ষ", "ক্ষ"},
    {kModeUnicode, "jmA", L"কর্ম", "কর্ম"},
    {kModeUnicode, "dj", L"কি", "কি"},
    {kModeUnicode, "cjf", L"কো", "কো"},
    {kModeUnicode, "jgN<", L"ক্", "ক্ষ + Backspace"},
    {kModeUnicode, "hfQVfW", L"বাংলায়", "বাংলায়"},
    {kModeClassic, "gfdm", L"Avwg", "Classic আমি"},
    {kModeClassic, "jfcu", L"Kv‡R", "Classic কাজে"},
    {kModeUnicode, "Ffdm", L"আমি", "অ + া = আ (আমি typed as Shift+F F D M)"},
    {kModeUnicode, "clM\n", L"দেশ", "দেশ (C L Shift+M) + Enter: final before app sees it"},
    {kModeUnicode, "jgN ", L"ক্ষ", "ক্ষ + Space: final before the app sees Space"},
    {kModeEnglish, "jfd", L"", "English mode passes keys through (not eaten)"},
};

void pump() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

std::wstring windowText(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

std::string hex(std::wstring_view text) {
    std::string out;
    char buf[12];
    for (wchar_t c : text) {
        std::snprintf(buf, sizeof buf, "%sU+%04X", out.empty() ? "" : " ", static_cast<unsigned>(c));
        out += buf;
    }
    return out.empty() ? "(empty)" : out;
}

void setShift(bool down) {
    BYTE state[256] = {};
    GetKeyboardState(state);
    state[VK_SHIFT] = state[VK_LSHIFT] = down ? 0x80 : 0;
    state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;
    state[VK_MENU] = state[VK_LMENU] = state[VK_RMENU] = 0;
    state[VK_CAPITAL] = 0;
    SetKeyboardState(state);
}

// Delivers one key to the text services; returns whether a text service ate it. For a key
// that is not eaten, `textBeforeApp` receives the control's text at the moment TSF hands
// the key back, i.e. what the application would see when it processes the key.
bool sendKey(ITfKeystrokeMgr* keystrokes, HWND edit, UINT vk, bool shift, std::wstring* textBeforeApp) {
    setShift(shift);
    const UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    const LPARAM down = 1 | (static_cast<LPARAM>(scan) << 16);
    const LPARAM up = down | (1LL << 30) | (1LL << 31);
    BOOL eaten = FALSE;
    keystrokes->TestKeyDown(vk, down, &eaten);
    if (eaten) keystrokes->KeyDown(vk, down, &eaten);
    if (!eaten && textBeforeApp != nullptr) *textBeforeApp = windowText(edit);
    BOOL upEaten = FALSE;
    keystrokes->TestKeyUp(vk, up, &upEaten);
    if (upEaten) keystrokes->KeyUp(vk, up, &upEaten);
    setShift(false);
    pump();
    return eaten != FALSE;
}

HRESULT setMode(ITfThreadMgr* threadMgr, TfClientId clientId, LONG mode) {
    ComPtr<ITfCompartmentMgr> global;
    HRESULT hr = threadMgr->GetGlobalCompartment(&global);
    ComPtr<ITfCompartment> compartment;
    if (SUCCEEDED(hr)) hr = global->GetCompartment(kGuidModeCompartment, &compartment);
    if (FAILED(hr)) return hr;
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_I4;
    value.lVal = mode;
    hr = compartment->SetValue(clientId, &value);
    pump();
    return hr;
}

LONG readMode(ITfThreadMgr* threadMgr) {
    ComPtr<ITfCompartmentMgr> global;
    ComPtr<ITfCompartment> compartment;
    VARIANT value;
    VariantInit(&value);
    if (SUCCEEDED(threadMgr->GetGlobalCompartment(&global)) &&
        SUCCEEDED(global->GetCompartment(kGuidModeCompartment, &compartment)) &&
        SUCCEEDED(compartment->GetValue(&value)) && value.vt == VT_I4) {
        return value.lVal;
    }
    return -1;
}

int fail(const char* what, HRESULT hr) {
    std::printf("SETUP FAILED: %s (hr=0x%08lX)\n", what, static_cast<unsigned long>(hr));
    return 2;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return fail("CoInitializeEx", hr);

    // july_tip_smoke --set-mode N: only write the shared mode (0 English, 1 Unicode,
    // 2 Classic) from this process, to check that other processes are notified.
    if (argc == 3 && std::wstring_view(argv[1]) == L"--set-mode") {
        ComPtr<ITfThreadMgr> threadMgr;
        hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&threadMgr));
        TfClientId clientId = TF_CLIENTID_NULL;
        if (SUCCEEDED(hr)) hr = threadMgr->Activate(&clientId);
        if (SUCCEEDED(hr)) hr = setMode(threadMgr.Get(), clientId, _wtoi(argv[2]));
        if (FAILED(hr)) return fail("set mode", hr);
        std::printf("mode set to %d\n", _wtoi(argv[2]));
        threadMgr->Deactivate();
        CoUninitialize();
        return 0;
    }

    if (LoadLibraryW(L"msftedit.dll") == nullptr) return fail("load msftedit.dll", HRESULT_FROM_WIN32(GetLastError()));
    HWND frame = CreateWindowExW(0, L"STATIC", L"july_tip_smoke", WS_OVERLAPPEDWINDOW, 100, 100, 600, 200, nullptr,
                                 nullptr, GetModuleHandleW(nullptr), nullptr);
    HWND edit = CreateWindowExW(0, MSFTEDIT_CLASS, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE, 0, 0, 580, 160, frame,
                                nullptr, GetModuleHandleW(nullptr), nullptr);
    if (frame == nullptr || edit == nullptr) return fail("create RichEdit", HRESULT_FROM_WIN32(GetLastError()));
    SendMessageW(edit, EM_SETEDITSTYLE, SES_USECTF, SES_USECTF);  // RichEdit talks to TSF
    ShowWindow(frame, SW_SHOWNOACTIVATE);
    SetFocus(edit);
    pump();

    ComPtr<ITfThreadMgr> threadMgr;
    hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&threadMgr));
    if (FAILED(hr)) return fail("create thread manager", hr);
    TfClientId clientId = TF_CLIENTID_NULL;
    hr = threadMgr->Activate(&clientId);
    if (FAILED(hr)) return fail("activate thread manager", hr);

    ComPtr<ITfInputProcessorProfileMgr> profiles;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&profiles));
    if (SUCCEEDED(hr)) {
        hr = profiles->ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR, kLangId, kClsidTextService, kGuidProfile, nullptr,
                                       TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
    }
    if (FAILED(hr)) return fail("activate July Bangla Keyboard profile (is it registered?)", hr);
    SetFocus(edit);
    pump();

    TF_INPUTPROCESSORPROFILE active{};
    if (SUCCEEDED(profiles->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &active))) {
        const bool ours = active.clsid == kClsidTextService && active.guidProfile == kGuidProfile;
        std::printf("active keyboard profile: %s (langid 0x%04X)\n", ours ? "July Bangla Keyboard" : "OTHER",
                    active.langid);
        if (!ours) return fail("our profile is not the active one", E_FAIL);
    }

    ComPtr<ITfKeystrokeMgr> keystrokes;
    hr = threadMgr.As(&keystrokes);
    if (FAILED(hr)) return fail("keystroke manager", hr);

    const LONG originalMode = readMode(threadMgr.Get());
    int failures = 0;
    for (const Case& c : kCases) {
        SetWindowTextW(edit, L"");
        hr = setMode(threadMgr.Get(), clientId, c.mode);
        if (FAILED(hr)) return fail("set mode compartment", hr);

        bool typingKeysEaten = true;   // Bijoy keys must be taken by the text service
        bool passThroughOk = true;     // Enter/Space must reach the app, after the commit
        bool anyEaten = false;
        for (const char* k = c.keys; *k != '\0'; ++k) {
            const bool passThrough = *k == '\n' || *k == ' ';
            const bool shift = *k >= 'A' && *k <= 'Z';
            const UINT vk = *k == '<'  ? VK_BACK
                            : *k == '\n' ? VK_RETURN
                            : *k == ' '  ? VK_SPACE
                                         : static_cast<UINT>(shift ? *k : *k - 'a' + 'A');
            std::wstring textBeforeApp;
            const bool eaten = sendKey(keystrokes.Get(), edit, vk, shift, &textBeforeApp);
            anyEaten = anyEaten || eaten;
            if (passThrough) {
                // The syllable must already be final when the application gets the key.
                passThroughOk = passThroughOk && !eaten && textBeforeApp == c.expected;
            } else {
                typingKeysEaten = typingKeysEaten && eaten;
            }
        }
        const std::wstring text = windowText(edit);
        // English mode must not eat keys (the application types them itself); here no
        // application handles them, so the control stays empty.
        const bool ok = c.mode == kModeEnglish ? (!anyEaten && text.empty())
                                               : (typingKeysEaten && passThroughOk && text == c.expected);
        std::printf("%s  %-46s got [%s]\n", ok ? "PASS" : "FAIL", c.name, hex(text).c_str());
        if (!ok) {
            std::printf("      expected [%s]%s%s\n", hex(c.expected).c_str(),
                        typingKeysEaten ? "" : " (some typing keys not eaten)",
                        passThroughOk ? "" : " (Enter/Space eaten, or text not final before the app saw it)");
            ++failures;
        }
    }

    // The input-mode button in Windows' input indicator must exist and follow the mode.
    {
        ComPtr<ITfLangBarItemMgr> langBar;
        ComPtr<ITfLangBarItemButton> button;
        if (SUCCEEDED(threadMgr.As(&langBar))) {
            ComPtr<IEnumTfLangBarItems> items;
            if (SUCCEEDED(langBar->EnumItems(&items))) {
                ComPtr<ITfLangBarItem> item;
                ULONG fetched = 0;
                while (!button && items->Next(1, &item, &fetched) == S_OK && fetched == 1) {
                    TF_LANGBARITEMINFO info{};
                    if (SUCCEEDED(item->GetInfo(&info)) && info.clsidService == kClsidTextService &&
                        info.guidItem == GUID_LBI_INPUTMODE) {
                        item.As(&button);
                    }
                    item.Reset();
                }
            }
        }
        struct LabelCase { LONG mode; const wchar_t* label; };
        const LabelCase labels[] = {{kModeEnglish, L"EN"},
                                    {kModeUnicode, L"বাংলা"},
                                    {kModeClassic, L"বিজয়"}};
        for (const LabelCase& l : labels) {
            setMode(threadMgr.Get(), clientId, l.mode);
            BSTR text = nullptr;
            const bool ok = button && SUCCEEDED(button->GetText(&text)) && text != nullptr && std::wstring_view(text) == l.label;
            std::printf("%s  input-indicator button shows mode %ld %-17s got [%s]\n", ok ? "PASS" : "FAIL", l.mode, "",
                        text ? hex(text).c_str() : (button ? "(no text)" : "(button not found)"));
            if (text) SysFreeString(text);
            failures += ok ? 0 : 1;
        }
    }

    // Leave the shared mode as we found it.
    setMode(threadMgr.Get(), clientId, originalMode < 0 ? kModeEnglish : originalMode);
    threadMgr->Deactivate();
    DestroyWindow(frame);
    CoUninitialize();
    std::printf("%d typing case(s) + 3 indicator checks, %d failure(s)\n", static_cast<int>(std::size(kCases)),
                failures);
    return failures == 0 ? 0 : 1;
}
