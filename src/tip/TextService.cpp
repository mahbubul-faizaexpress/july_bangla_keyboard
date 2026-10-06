#include "TextService.h"

#include <iterator>
#include <new>

#include "Guids.h"
#include "Settings.h"

using Microsoft::WRL::ComPtr;

namespace july::tip {

namespace {

// Edit session carrying one EditResult. For synchronous requests it lives on the caller's
// stack (no allocation per keystroke): TSF runs DoEditSession before RequestEditSession
// returns and does not keep the object. Only the asynchronous fallback heap-allocates.
class EditSession final : public ITfEditSession {
public:
    EditSession(TextService* service, ITfContext* context, const EditResult& result, bool heap) noexcept
        : service_(service), context_(context), result_(result), refs_(heap ? 1 : 0), heap_(heap) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (ppv == nullptr) return E_POINTER;
        if (riid == IID_IUnknown || riid == __uuidof(ITfEditSession)) {
            *ppv = static_cast<ITfEditSession*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        const ULONG refs = --refs_;
        if (refs == 0 && heap_) delete this;
        return refs;
    }
    STDMETHODIMP DoEditSession(TfEditCookie ec) override {
        return service_->applyInSession(ec, context_.Get(), result_);
    }

    ULONG refs() const noexcept { return refs_; }

private:
    ~EditSession() = default;
    friend class TextService;

    TextService* service_;  // kept alive by the caller for the session's duration
    ComPtr<ITfContext> context_;
    EditResult result_;
    std::atomic<ULONG> refs_;
    bool heap_;
};

bool keyDown(int vk) noexcept { return (GetKeyState(vk) & 0x8000) != 0; }

bool isModifierKey(WPARAM vk) noexcept {
    switch (vk) {
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_MENU: case VK_LMENU: case VK_RMENU:
    case VK_LWIN: case VK_RWIN: case VK_CAPITAL: case VK_NUMLOCK: case VK_SCROLL:
        return true;
    default:
        return false;
    }
}

// Scan code from WM_KEYDOWN lParam; extended keys (arrows, Insert, numpad Enter, ...) are
// moved out of the layout's range so they are never mistaken for typing keys.
std::uint16_t scanCode(LPARAM lParam) noexcept {
    const auto scan = static_cast<std::uint16_t>((lParam >> 16) & 0xFF);
    const bool extended = ((lParam >> 24) & 1) != 0;
    return extended ? static_cast<std::uint16_t>(scan | 0x100) : scan;
}

ComposerOptions optionsFor(InputMode mode) noexcept {
    ComposerOptions options;
    options.encoding = mode == InputMode::Classic ? OutputEncoding::Classic : OutputEncoding::Unicode;
    return options;
}

} // namespace

// --- IUnknown --------------------------------------------------------------------------

STDMETHODIMP TextService::QueryInterface(REFIID riid, void** ppv) {
    if (ppv == nullptr) return E_POINTER;
    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ITfTextInputProcessor) || riid == __uuidof(ITfTextInputProcessorEx)) {
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (riid == __uuidof(ITfThreadMgrEventSink)) {
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (riid == __uuidof(ITfKeyEventSink)) {
        *ppv = static_cast<ITfKeyEventSink*>(this);
    } else if (riid == __uuidof(ITfCompositionSink)) {
        *ppv = static_cast<ITfCompositionSink*>(this);
    } else if (riid == __uuidof(ITfCompartmentEventSink)) {
        *ppv = static_cast<ITfCompartmentEventSink*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) TextService::AddRef() { return ++refs_; }

STDMETHODIMP_(ULONG) TextService::Release() {
    const ULONG refs = --refs_;
    if (refs == 0) delete this;
    return refs;
}

// --- Activation ------------------------------------------------------------------------

STDMETHODIMP TextService::Activate(ITfThreadMgr* threadMgr, TfClientId clientId) {
    return ActivateEx(threadMgr, clientId, 0);
}

STDMETHODIMP TextService::ActivateEx(ITfThreadMgr* threadMgr, TfClientId clientId, DWORD) {
    if (threadMgr == nullptr) return E_INVALIDARG;
    threadMgr_ = threadMgr;
    clientId_ = clientId;

    ComPtr<ITfSource> source;
    if (SUCCEEDED(threadMgr_.As(&source))) {
        source->AdviseSink(__uuidof(ITfThreadMgrEventSink), static_cast<ITfThreadMgrEventSink*>(this),
                           &threadMgrSinkCookie_);
    }

    ComPtr<ITfKeystrokeMgr> keystrokes;
    HRESULT hr = threadMgr_.As(&keystrokes);
    if (SUCCEEDED(hr)) hr = keystrokes->AdviseKeyEventSink(clientId_, static_cast<ITfKeyEventSink*>(this), TRUE);
    keySinkAdvised_ = SUCCEEDED(hr);
    if (FAILED(hr)) {
        Deactivate();
        return hr;
    }

    const TF_PRESERVEDKEY cycleKey{'B', TF_MOD_CONTROL | TF_MOD_ALT};
    static constexpr wchar_t kCycleDescription[] = L"Switch input mode (English / Bangla / Bijoy)";
    keyPreserved_ = SUCCEEDED(keystrokes->PreserveKey(clientId_, kGuidPreservedKeyCycle, &cycleKey, kCycleDescription,
                                                      static_cast<ULONG>(std::size(kCycleDescription) - 1)));

    // The input mode lives in a global compartment so every TSF thread in every process
    // sees the same mode and is notified when it changes.
    ComPtr<ITfCompartmentMgr> globalCompartments;
    if (SUCCEEDED(threadMgr_->GetGlobalCompartment(&globalCompartments)) &&
        SUCCEEDED(globalCompartments->GetCompartment(kGuidModeCompartment, &modeCompartment_))) {
        ComPtr<ITfSource> compartmentSource;
        if (SUCCEEDED(modeCompartment_.As(&compartmentSource))) {
            compartmentSource->AdviseSink(__uuidof(ITfCompartmentEventSink),
                                          static_cast<ITfCompartmentEventSink*>(this), &compartmentSinkCookie_);
        }
    }
    // First text service in this session: restore the user's last mode (cold path, once
    // per activation). Sandboxed (AppContainer) apps cannot read it and start in English.
    InputMode mode = InputMode::English;
    if (!readModeCompartment(mode)) {
        mode = loadSettings().mode;
        if (modeCompartment_) writeModeCompartment(mode);
    }
    mode_ = mode;
    composer_ = Composer(optionsFor(mode_));
    return S_OK;
}

STDMETHODIMP TextService::Deactivate() {
    commitComposition();
    composition_.Reset();
    composer_.reset();

    if (modeCompartment_ && compartmentSinkCookie_ != TF_INVALID_COOKIE) {
        ComPtr<ITfSource> source;
        if (SUCCEEDED(modeCompartment_.As(&source))) source->UnadviseSink(compartmentSinkCookie_);
    }
    compartmentSinkCookie_ = TF_INVALID_COOKIE;
    modeCompartment_.Reset();

    if (threadMgr_) {
        ComPtr<ITfKeystrokeMgr> keystrokes;
        if (SUCCEEDED(threadMgr_.As(&keystrokes))) {
            if (keyPreserved_) {
                const TF_PRESERVEDKEY cycleKey{'B', TF_MOD_CONTROL | TF_MOD_ALT};
                keystrokes->UnpreserveKey(kGuidPreservedKeyCycle, &cycleKey);
            }
            if (keySinkAdvised_) keystrokes->UnadviseKeyEventSink(clientId_);
        }
        if (threadMgrSinkCookie_ != TF_INVALID_COOKIE) {
            ComPtr<ITfSource> source;
            if (SUCCEEDED(threadMgr_.As(&source))) source->UnadviseSink(threadMgrSinkCookie_);
        }
    }
    keyPreserved_ = false;
    keySinkAdvised_ = false;
    threadMgrSinkCookie_ = TF_INVALID_COOKIE;
    threadMgr_.Reset();
    clientId_ = TF_CLIENTID_NULL;
    return S_OK;
}

// --- Focus and composition lifetime ----------------------------------------------------

STDMETHODIMP TextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) {
    // Never carry a half-typed syllable into another document.
    commitComposition();
    composition_.Reset();
    composer_.reset();
    return S_OK;
}

STDMETHODIMP TextService::OnCompositionTerminated(TfEditCookie, ITfComposition*) {
    // The application finalized whatever is displayed; drop our state, keep its text.
    composition_.Reset();
    composer_.reset();
    return S_OK;
}

// --- Keys ------------------------------------------------------------------------------

STDMETHODIMP TextService::OnTestKeyDown(ITfContext*, WPARAM wParam, LPARAM lParam, BOOL* eaten) {
    if (eaten == nullptr) return E_INVALIDARG;
    *eaten = FALSE;
    if (mode_ == InputMode::English || isModifierKey(wParam)) return S_OK;

    if (wParam == VK_BACK || wParam == VK_ESCAPE) {
        // Edit or cancel our own syllable; otherwise the application handles the key.
        *eaten = composer_.composing() ? TRUE : FALSE;
        return S_OK;
    }
    if (!keyDown(VK_CONTROL) && !keyDown(VK_MENU)) {
        // Predict on a copy, so the engine itself is not changed here.
        Composer probe = composer_;
        if (probe.pressKey(scanCode(lParam), keyDown(VK_SHIFT)).eaten) {
            *eaten = TRUE;
            return S_OK;
        }
    }

    // The key goes to the application (Enter, Space, arrows, shortcuts). Finalize the open
    // syllable now, before the key is reported as not eaten. If the key were first claimed
    // and only released in OnKeyDown, Chromium/Electron applications treat it as an IME
    // key and can act on it (e.g. send a chat message on Enter) before the committed text
    // arrives, losing the last syllable.
    if (composer_.composing()) commitComposition();
    return S_OK;
}

EditResult TextService::process(WPARAM vk, LPARAM lParam) noexcept {
    if (vk == VK_BACK) return composer_.backspace();
    if (vk == VK_ESCAPE && composer_.composing()) {
        // Cancel: remove the uncommitted syllable from the document.
        composer_.reset();
        EditResult cancel;
        cancel.eaten = true;
        return cancel;
    }
    if (keyDown(VK_CONTROL) || keyDown(VK_MENU)) {
        EditResult result = composer_.commitAll();  // shortcut: finalize, then let it through
        result.eaten = false;
        return result;
    }
    return composer_.pressKey(scanCode(lParam), keyDown(VK_SHIFT));
}

STDMETHODIMP TextService::OnKeyDown(ITfContext* context, WPARAM wParam, LPARAM lParam, BOOL* eaten) {
    if (eaten == nullptr) return E_INVALIDARG;
    *eaten = FALSE;
    if (mode_ == InputMode::English || isModifierKey(wParam) || context == nullptr) return S_OK;

    const EditResult result = process(wParam, lParam);
    if (FAILED(apply(context, result))) {
        // Fail safe: drop engine state, never swallow the user's key.
        composer_.reset();
        composition_.Reset();
        return S_OK;
    }
    *eaten = result.eaten ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    if (eaten == nullptr) return E_INVALIDARG;
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* eaten) {
    if (eaten == nullptr) return E_INVALIDARG;
    *eaten = FALSE;
    return S_OK;
}

STDMETHODIMP TextService::OnPreservedKey(ITfContext*, REFGUID guid, BOOL* eaten) {
    if (eaten == nullptr) return E_INVALIDARG;
    *eaten = FALSE;
    if (guid != kGuidPreservedKeyCycle) return S_OK;
    // Writing the global compartment notifies every TIP instance, including this one.
    if (SUCCEEDED(writeModeCompartment(nextMode(mode_)))) *eaten = TRUE;
    return S_OK;
}

// --- Mode ------------------------------------------------------------------------------

bool TextService::readModeCompartment(InputMode& mode) const noexcept {
    if (!modeCompartment_) return false;
    VARIANT value;
    VariantInit(&value);
    const bool set = SUCCEEDED(modeCompartment_->GetValue(&value)) && value.vt == VT_I4;
    if (set) mode = modeFromInt(value.lVal);
    VariantClear(&value);
    return set;
}

HRESULT TextService::writeModeCompartment(InputMode mode) noexcept {
    if (!modeCompartment_) {
        setMode(mode);  // no compartment: keep the mode local to this thread
        return S_OK;
    }
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_I4;
    value.lVal = static_cast<LONG>(mode);
    return modeCompartment_->SetValue(clientId_, &value);
}

STDMETHODIMP TextService::OnChange(REFGUID guid) {
    InputMode mode = mode_;
    if (guid == kGuidModeCompartment && readModeCompartment(mode)) setMode(mode);
    return S_OK;
}

void TextService::setMode(InputMode mode) noexcept {
    if (mode == mode_) return;
    commitComposition();
    mode_ = mode;
    composer_ = Composer(optionsFor(mode_));
}

// --- Writing text ----------------------------------------------------------------------

void TextService::commitComposition() noexcept {
    if (!composition_) {
        composer_.reset();
        return;
    }
    ComPtr<ITfRange> range;
    ComPtr<ITfContext> context;
    if (FAILED(composition_->GetRange(&range)) || FAILED(range->GetContext(&context))) {
        composition_.Reset();
        composer_.reset();
        return;
    }
    const EditResult result = composer_.commitAll();
    apply(context.Get(), result);
}

HRESULT TextService::apply(ITfContext* context, const EditResult& result) noexcept {
    if (!composition_ && result.commit.empty() && result.composition.empty()) return S_OK;

    // Synchronous so the text lands before any key we pass on to the application.
    EditSession session(this, context, result, /*heap=*/false);
    session.AddRef();
    HRESULT sessionResult = E_FAIL;
    HRESULT hr = context->RequestEditSession(clientId_, &session, TF_ES_SYNC | TF_ES_READWRITE, &sessionResult);
    const ULONG remaining = session.Release();
    if (remaining != 0) return E_UNEXPECTED;  // TSF must not keep a synchronous session
    if (SUCCEEDED(hr) && sessionResult != TF_E_SYNCHRONOUS) return sessionResult;

    // The document could not be locked synchronously: queue it (ordering is preserved by
    // TSF's edit-session queue).
    auto* queued = new (std::nothrow) EditSession(this, context, result, /*heap=*/true);
    if (queued == nullptr) return E_OUTOFMEMORY;
    hr = context->RequestEditSession(clientId_, queued, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &sessionResult);
    queued->Release();
    return hr;
}

HRESULT TextService::applyInSession(TfEditCookie ec, ITfContext* context, const EditResult& result) noexcept {
    TextBuffer<kTextCapacity * 2> text;
    text.append(result.commit.view());
    text.append(result.composition.view());

    ComPtr<ITfRange> range;
    if (composition_) {
        HRESULT hr = composition_->GetRange(&range);
        if (FAILED(hr)) return hr;
    } else {
        if (text.empty()) return S_OK;
        TF_SELECTION selection{};
        ULONG fetched = 0;
        HRESULT hr = context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
        if (FAILED(hr) || fetched != 1) return FAILED(hr) ? hr : E_FAIL;
        ComPtr<ITfRange> selectionRange;
        selectionRange.Attach(selection.range);

        ComPtr<ITfContextComposition> contextComposition;
        hr = context->QueryInterface(IID_PPV_ARGS(&contextComposition));
        if (FAILED(hr)) return hr;
        hr = contextComposition->StartComposition(ec, selectionRange.Get(), static_cast<ITfCompositionSink*>(this),
                                                  &composition_);
        if (FAILED(hr) || !composition_) return FAILED(hr) ? hr : E_FAIL;  // the app refused a composition
        hr = composition_->GetRange(&range);
        if (FAILED(hr)) return hr;
    }

    const std::u16string_view all = text.view();
    HRESULT hr = range->SetText(ec, 0, reinterpret_cast<const WCHAR*>(all.data()), static_cast<LONG>(all.size()));
    if (FAILED(hr)) return hr;

    // Put the caret at the end of the inserted text.
    ComPtr<ITfRange> caret;
    if (SUCCEEDED(range->Clone(&caret)) && SUCCEEDED(caret->Collapse(ec, TF_ANCHOR_END))) {
        TF_SELECTION selection{};
        selection.range = caret.Get();
        selection.style.ase = TF_AE_NONE;
        selection.style.fInterimChar = FALSE;
        context->SetSelection(ec, 1, &selection);
    }

    if (result.composition.empty()) {
        // Everything is final: end the composition.
        hr = composition_->EndComposition(ec);
        composition_.Reset();
        return hr;
    }
    if (!result.commit.empty()) {
        // The committed prefix leaves the composition; the rest stays editable.
        ComPtr<ITfRange> tail;
        LONG moved = 0;
        hr = range->Clone(&tail);
        if (SUCCEEDED(hr)) hr = tail->ShiftStart(ec, static_cast<LONG>(result.commit.size()), &moved, nullptr);
        if (SUCCEEDED(hr)) hr = composition_->ShiftStart(ec, tail.Get());
        if (FAILED(hr)) return hr;
    }
    return S_OK;
}

} // namespace july::tip
