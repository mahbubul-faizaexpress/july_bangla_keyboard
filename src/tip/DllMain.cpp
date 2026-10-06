// TSF text input processor DLL entry points.
// Phase 1 skeleton: exports exist so the build, .def file and both architectures are
// exercised. The TextService class factory and registration arrive in Phase 5.

#include <windows.h>

namespace {
HINSTANCE g_module = nullptr;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID /*clsid*/, _In_ REFIID /*iid*/,
                                        _Outptr_ LPVOID* object) {
    if (object == nullptr) return E_POINTER;
    *object = nullptr;
    return CLASS_E_CLASSNOTAVAILABLE;
}

__control_entrypoint(DllExport) STDAPI DllCanUnloadNow() {
    return S_OK;
}

STDAPI DllRegisterServer() {
    return E_NOTIMPL;
}

STDAPI DllUnregisterServer() {
    return E_NOTIMPL;
}
