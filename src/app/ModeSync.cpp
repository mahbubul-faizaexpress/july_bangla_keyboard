#include "ModeSync.h"

using Microsoft::WRL::ComPtr;

namespace july::app {

namespace {

// Must match src/tip/Guids.h (kGuidModeCompartment).
constexpr GUID kGuidModeCompartment = {
    0x540A2534, 0xDA60, 0x41E0, {0xA9, 0x2C, 0xF5, 0x2B, 0x10, 0x13, 0x9F, 0x44}};

} // namespace

STDMETHODIMP ModeSync::QueryInterface(REFIID riid, void** ppv) {
    if (ppv == nullptr) return E_POINTER;
    if (riid == IID_IUnknown || riid == __uuidof(ITfCompartmentEventSink)) {
        *ppv = static_cast<ITfCompartmentEventSink*>(this);
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT ModeSync::start(Listener listener, void* context, InputMode initialMode) noexcept {
    listener_ = listener;
    context_ = context;

    HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&threadMgr_));
    if (SUCCEEDED(hr)) hr = threadMgr_->Activate(&clientId_);
    ComPtr<ITfCompartmentMgr> global;
    if (SUCCEEDED(hr)) hr = threadMgr_->GetGlobalCompartment(&global);
    if (SUCCEEDED(hr)) hr = global->GetCompartment(kGuidModeCompartment, &compartment_);
    ComPtr<ITfSource> source;
    if (SUCCEEDED(hr)) hr = compartment_.As(&source);
    if (SUCCEEDED(hr)) hr = source->AdviseSink(__uuidof(ITfCompartmentEventSink), static_cast<ITfCompartmentEventSink*>(this), &cookie_);
    if (FAILED(hr)) {
        stop();
        mode_ = initialMode;
        return hr;
    }

    // A running session already has a mode (set by a text service or an earlier run);
    // otherwise restore the user's last mode.
    InputMode current = InputMode::English;
    if (readCompartment(current)) {
        mode_ = current;
    } else {
        mode_ = initialMode;
        setMode(initialMode);
    }
    return S_OK;
}

void ModeSync::stop() noexcept {
    if (compartment_ && cookie_ != TF_INVALID_COOKIE) {
        ComPtr<ITfSource> source;
        if (SUCCEEDED(compartment_.As(&source))) source->UnadviseSink(cookie_);
    }
    cookie_ = TF_INVALID_COOKIE;
    compartment_.Reset();
    if (threadMgr_ && clientId_ != TF_CLIENTID_NULL) threadMgr_->Deactivate();
    clientId_ = TF_CLIENTID_NULL;
    threadMgr_.Reset();
}

bool ModeSync::readCompartment(InputMode& mode) const noexcept {
    if (!compartment_) return false;
    VARIANT value;
    VariantInit(&value);
    const bool ok = SUCCEEDED(compartment_->GetValue(&value)) && value.vt == VT_I4;
    if (ok) mode = modeFromInt(value.lVal);
    VariantClear(&value);
    return ok;
}

HRESULT ModeSync::setMode(InputMode mode) noexcept {
    if (!compartment_) {
        // TSF unavailable: keep the UI consistent locally.
        mode_ = mode;
        if (listener_ != nullptr) listener_(context_, mode_);
        return S_OK;
    }
    VARIANT value;
    VariantInit(&value);
    value.vt = VT_I4;
    value.lVal = static_cast<LONG>(mode);
    return compartment_->SetValue(clientId_, &value);  // OnChange reports it back
}

STDMETHODIMP ModeSync::OnChange(REFGUID guid) {
    if (guid != kGuidModeCompartment) return S_OK;
    InputMode mode = mode_;
    if (readCompartment(mode) && mode != mode_) {
        mode_ = mode;
        if (listener_ != nullptr) listener_(context_, mode_);
    }
    return S_OK;
}

} // namespace july::app
