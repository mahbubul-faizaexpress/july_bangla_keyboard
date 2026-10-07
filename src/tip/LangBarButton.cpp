#include "LangBarButton.h"

#include <ctffunc.h>
#include <olectl.h>

#include "Guids.h"
#include "ModeVisuals.h"

namespace july::tip {

namespace {
constexpr DWORD kSinkCookie = 0x4A42;  // single sink
}

STDMETHODIMP LangBarButton::QueryInterface(REFIID riid, void** ppv) {
    if (ppv == nullptr) return E_POINTER;
    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ITfLangBarItem) || riid == __uuidof(ITfLangBarItemButton)) {
        *ppv = static_cast<ITfLangBarItemButton*>(this);
    } else if (riid == __uuidof(ITfSource)) {
        *ppv = static_cast<ITfSource*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) LangBarButton::Release() {
    const ULONG refs = --refs_;
    if (refs == 0) delete this;
    return refs;
}

void LangBarButton::setMode(InputMode mode) noexcept {
    mode_ = mode;
    if (sink_) sink_->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

STDMETHODIMP LangBarButton::GetInfo(TF_LANGBARITEMINFO* info) {
    if (info == nullptr) return E_INVALIDARG;
    *info = {};
    info->clsidService = kClsidTextService;
    info->guidItem = GUID_LBI_INPUTMODE;
    info->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    info->ulSort = 0;
    (void)lstrcpynW(info->szDescription, L"July Bangla Keyboard input mode", TF_LBI_DESC_MAXLEN);
    return S_OK;
}

STDMETHODIMP LangBarButton::GetStatus(DWORD* status) {
    if (status == nullptr) return E_INVALIDARG;
    *status = 0;
    return S_OK;
}

STDMETHODIMP LangBarButton::GetTooltipString(BSTR* tooltip) {
    if (tooltip == nullptr) return E_INVALIDARG;
    *tooltip = SysAllocString(visualFor(mode_).tooltip);
    return *tooltip != nullptr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetText(BSTR* text) {
    if (text == nullptr) return E_INVALIDARG;
    *text = SysAllocString(visualFor(mode_).label);
    return *text != nullptr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetIcon(HICON* icon) {
    if (icon == nullptr) return E_INVALIDARG;
    *icon = createModeIcon(mode_, GetDpiForSystem());  // the caller destroys it
    return *icon != nullptr ? S_OK : E_FAIL;
}

STDMETHODIMP LangBarButton::OnClick(TfLBIClick click, POINT, const RECT*) {
    if (click == TF_LBI_CLK_LEFT && onClick_ != nullptr) onClick_(context_);
    return S_OK;
}

STDMETHODIMP LangBarButton::AdviseSink(REFIID riid, IUnknown* sink, DWORD* cookie) {
    if (sink == nullptr || cookie == nullptr) return E_INVALIDARG;
    if (riid != __uuidof(ITfLangBarItemSink)) return CONNECT_E_CANNOTCONNECT;
    if (sink_) return CONNECT_E_ADVISELIMIT;
    const HRESULT hr = sink->QueryInterface(IID_PPV_ARGS(&sink_));
    if (FAILED(hr)) return E_NOINTERFACE;
    *cookie = kSinkCookie;
    return S_OK;
}

STDMETHODIMP LangBarButton::UnadviseSink(DWORD cookie) {
    if (cookie != kSinkCookie || !sink_) return CONNECT_E_NOCONNECTION;
    sink_.Reset();
    return S_OK;
}

} // namespace july::tip
