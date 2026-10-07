#pragma once

#include <windows.h>
#include <msctf.h>
#include <wrl/client.h>

#include <atomic>

#include "LangBarButton.h"
#include "Module.h"
#include "july/engine/Composer.h"
#include "july/engine/InputMode.h"

namespace july::tip {

// The TSF text input processor. TSF creates one instance per thread manager (per UI
// thread that uses TSF), inside the application's process. Every callback arrives on that
// thread, so the instance needs no locking; the engine state (Composer) is per instance.
class TextService final : public ITfTextInputProcessorEx,
                          public ITfThreadMgrEventSink,
                          public ITfKeyEventSink,
                          public ITfCompositionSink,
                          public ITfCompartmentEventSink,
                          public ITfTextEditSink {
public:
    TextService() noexcept = default;
    TextService(const TextService&) = delete;
    TextService& operator=(const TextService&) = delete;

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor / ITfTextInputProcessorEx
    STDMETHODIMP Activate(ITfThreadMgr* threadMgr, TfClientId clientId) override;
    STDMETHODIMP ActivateEx(ITfThreadMgr* threadMgr, TfClientId clientId, DWORD flags) override;
    STDMETHODIMP Deactivate() override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* previous) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL) override { return S_OK; }
    STDMETHODIMP OnTestKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* context, REFGUID guid, BOOL* eaten) override;

    // ITfCompositionSink: the application ended our composition (click elsewhere, etc.).
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ec, ITfComposition* composition) override;

    // ITfCompartmentEventSink: the input mode changed (in this or any other process).
    STDMETHODIMP OnChange(REFGUID guid) override;

    // ITfTextEditSink: the caret left our composition (mouse click, arrow keys handled by
    // the application, programmatic edits) -> finalize it where it is.
    STDMETHODIMP OnEndEdit(ITfContext* context, TfEditCookie ecReadOnly, ITfEditRecord* record) override;

    // Called from inside an edit session.
    HRESULT applyInSession(TfEditCookie ec, ITfContext* context, const EditResult& result) noexcept;

private:
    ~TextService() = default;

    EditResult process(WPARAM vk, LPARAM lParam) noexcept;
    HRESULT apply(ITfContext* context, const EditResult& result) noexcept;
    void commitComposition() noexcept;
    void watchDocument(ITfDocumentMgr* documentMgr) noexcept;  // (re)advise the text edit sink
    void unwatchDocument() noexcept;
    void setMode(InputMode mode) noexcept;
    static void onLangBarClick(void* self) noexcept;
    bool readModeCompartment(InputMode& mode) const noexcept;  // false if no mode is set yet
    HRESULT writeModeCompartment(InputMode mode) noexcept;

    std::atomic<ULONG> refs_{1};
    ObjectCount objectCount_;

    Microsoft::WRL::ComPtr<ITfThreadMgr> threadMgr_;
    TfClientId clientId_ = TF_CLIENTID_NULL;
    DWORD threadMgrSinkCookie_ = TF_INVALID_COOKIE;
    bool keySinkAdvised_ = false;
    bool keyPreserved_ = false;
    Microsoft::WRL::ComPtr<ITfCompartment> modeCompartment_;
    DWORD compartmentSinkCookie_ = TF_INVALID_COOKIE;
    Microsoft::WRL::ComPtr<ITfContext> watchedContext_;  // context with our text edit sink
    DWORD textEditSinkCookie_ = TF_INVALID_COOKIE;

    Microsoft::WRL::ComPtr<ITfComposition> composition_;  // our open composition, if any
    InputMode mode_ = InputMode::English;
    Composer composer_;
    LangBarButton* langBarButton_ = nullptr;  // owned reference; released in Deactivate
};

} // namespace july::tip
