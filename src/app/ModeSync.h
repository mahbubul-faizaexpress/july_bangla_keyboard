#pragma once

#include <windows.h>
#include <msctf.h>
#include <wrl/client.h>

#include "july/engine/InputMode.h"

namespace july::app {

// Reads, writes and observes the input mode in the TSF global compartment shared with
// every July Bangla text service instance (all applications on this desktop). Changes
// made with Ctrl+Alt+B in any application arrive through OnChange on this thread's
// message loop; no polling.
class ModeSync final : public ITfCompartmentEventSink {
public:
    using Listener = void (*)(void* context, InputMode mode);

    // COM must be initialized (STA) on the calling thread, which must pump messages.
    HRESULT start(Listener listener, void* context, InputMode initialMode) noexcept;
    void stop() noexcept;

    [[nodiscard]] InputMode mode() const noexcept { return mode_; }
    HRESULT setMode(InputMode mode) noexcept;

    // IUnknown: static lifetime (owned by the application object), so no ref counting.
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }

    // ITfCompartmentEventSink
    STDMETHODIMP OnChange(REFGUID guid) override;

private:
    bool readCompartment(InputMode& mode) const noexcept;

    Microsoft::WRL::ComPtr<ITfThreadMgr> threadMgr_;
    Microsoft::WRL::ComPtr<ITfCompartment> compartment_;
    TfClientId clientId_ = TF_CLIENTID_NULL;
    DWORD cookie_ = TF_INVALID_COOKIE;
    InputMode mode_ = InputMode::English;
    Listener listener_ = nullptr;
    void* context_ = nullptr;
};

} // namespace july::app
