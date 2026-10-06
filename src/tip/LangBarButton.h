#pragma once

#include <windows.h>
#include <msctf.h>
#include <wrl/client.h>

#include <atomic>

#include "Module.h"
#include "july/engine/InputMode.h"

namespace july::tip {

// The input-mode button (GUID_LBI_INPUTMODE): Windows shows it in its own input indicator
// next to the clock, so the mode is visible in every application without the companion.
// Clicking it moves to the next mode.
class LangBarButton final : public ITfLangBarItemButton, public ITfSource {
public:
    using ClickHandler = void (*)(void* context);

    LangBarButton(ClickHandler onClick, void* context) noexcept : onClick_(onClick), context_(context) {}
    LangBarButton(const LangBarButton&) = delete;
    LangBarButton& operator=(const LangBarButton&) = delete;

    void setMode(InputMode mode) noexcept;
    void detach() noexcept { onClick_ = nullptr; context_ = nullptr; }  // owner deactivating

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override;

    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* info) override;
    STDMETHODIMP GetStatus(DWORD* status) override;
    STDMETHODIMP Show(BOOL) override { return S_OK; }
    STDMETHODIMP GetTooltipString(BSTR* tooltip) override;

    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* area) override;
    STDMETHODIMP InitMenu(ITfMenu*) override { return E_NOTIMPL; }
    STDMETHODIMP OnMenuSelect(UINT) override { return E_NOTIMPL; }
    STDMETHODIMP GetIcon(HICON* icon) override;
    STDMETHODIMP GetText(BSTR* text) override;

    // ITfSource (one ITfLangBarItemSink)
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* sink, DWORD* cookie) override;
    STDMETHODIMP UnadviseSink(DWORD cookie) override;

private:
    ~LangBarButton() = default;

    std::atomic<ULONG> refs_{1};
    ObjectCount objectCount_;
    ClickHandler onClick_;
    void* context_;
    InputMode mode_ = InputMode::English;
    Microsoft::WRL::ComPtr<ITfLangBarItemSink> sink_;
};

} // namespace july::tip
