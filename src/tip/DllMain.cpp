// TSF text input processor DLL entry points and class factory.

#include <windows.h>

#include <new>

#include "Guids.h"
#include "Module.h"
#include "TextService.h"

using july::tip::Module;

namespace {

class ClassFactory final : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (ppv == nullptr) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    // The factory is a static singleton; its lifetime is the DLL's.
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }

    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (ppv == nullptr) return E_POINTER;
        *ppv = nullptr;
        if (outer != nullptr) return CLASS_E_NOAGGREGATION;
        auto* service = new (std::nothrow) july::tip::TextService();
        if (service == nullptr) return E_OUTOFMEMORY;
        const HRESULT hr = service->QueryInterface(riid, ppv);
        service->Release();  // drop the construction reference
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override {
        if (lock) ++Module::locks;
        else --Module::locks;
        return S_OK;
    }
};

ClassFactory g_factory;

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        Module::instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID iid, _Outptr_ LPVOID* object) {
    if (object == nullptr) return E_POINTER;
    *object = nullptr;
    if (clsid != july::tip::kClsidTextService) return CLASS_E_CLASSNOTAVAILABLE;
    return g_factory.QueryInterface(iid, object);
}

__control_entrypoint(DllExport) STDAPI DllCanUnloadNow() {
    return (Module::objects == 0 && Module::locks == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
    return july::tip::registerServer();
}

STDAPI DllUnregisterServer() {
    return july::tip::unregisterServer();
}
