// COM and TSF registration (DllRegisterServer / DllUnregisterServer).
//
// Requires administrator rights: COM classes are registered under HKLM\SOFTWARE\Classes
// and TSF profiles under HKLM\SOFTWARE\Microsoft\CTF. Registration uses only documented
// APIs (ITfInputProcessorProfileMgr, ITfCategoryMgr). A failed registration is rolled
// back so no half-registered text service is left behind.

#include <windows.h>
#include <msctf.h>
#include <wrl/client.h>

#include <iterator>

#include "Guids.h"
#include "Module.h"

using Microsoft::WRL::ComPtr;

namespace july::tip {

namespace {

// Category registrations (Windows 8+ requirements for a keyboard TIP).
const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,
    &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    &GUID_TFCAT_TIPCAP_SECUREMODE,
    &GUID_TFCAT_TIPCAP_COMLESS,
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
};

// US English layout used for English mode and for keys the engine passes through.
const HKL kSubstituteLayout = reinterpret_cast<HKL>(static_cast<ULONG_PTR>(0x04090409));

class ComScope {
public:
    ComScope() noexcept : hr_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComScope() {
        if (SUCCEEDED(hr_)) CoUninitialize();
    }
    ComScope(const ComScope&) = delete;
    ComScope& operator=(const ComScope&) = delete;

private:
    HRESULT hr_;
};

void clsidKeyPath(wchar_t (&buffer)[128]) noexcept {
    wchar_t clsid[64] = {};
    StringFromGUID2(kClsidTextService, clsid, static_cast<int>(std::size(clsid)));
    swprintf_s(buffer, L"SOFTWARE\\Classes\\CLSID\\%s", clsid);
}

// Full path of this DLL; 0 on failure (including truncation).
DWORD modulePath(wchar_t (&path)[MAX_PATH]) noexcept {
    const DWORD length = GetModuleFileNameW(Module::instance, path, MAX_PATH);
    return (length == 0 || length >= MAX_PATH) ? 0 : length;
}

HRESULT registerComServer() noexcept {
    wchar_t path[MAX_PATH] = {};
    const DWORD length = modulePath(path);
    if (length == 0) return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

    wchar_t key[128] = {};
    clsidKeyPath(key);
    LSTATUS status = RegSetKeyValueW(HKEY_LOCAL_MACHINE, key, nullptr, REG_SZ, kDisplayName, sizeof(kDisplayName));
    if (status != ERROR_SUCCESS) return HRESULT_FROM_WIN32(status);

    wchar_t inproc[160] = {};
    swprintf_s(inproc, L"%s\\InprocServer32", key);
    status = RegSetKeyValueW(HKEY_LOCAL_MACHINE, inproc, nullptr, REG_SZ, path,
                             static_cast<DWORD>((length + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS) return HRESULT_FROM_WIN32(status);
    static constexpr wchar_t kApartment[] = L"Apartment";
    status = RegSetKeyValueW(HKEY_LOCAL_MACHINE, inproc, L"ThreadingModel", REG_SZ, kApartment, sizeof(kApartment));
    return HRESULT_FROM_WIN32(status);
}

HRESULT unregisterComServer() noexcept {
    wchar_t key[128] = {};
    clsidKeyPath(key);
    const LSTATUS status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, key);
    return (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND) ? S_OK : HRESULT_FROM_WIN32(status);
}

HRESULT registerProfile() noexcept {
    ComPtr<ITfInputProcessorProfileMgr> profiles;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) return hr;

    wchar_t path[MAX_PATH] = {};
    const DWORD length = modulePath(path);
    if (length == 0) return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
    return profiles->RegisterProfile(kClsidTextService, kLangIdBanglaBangladesh, kGuidProfile, kDisplayName,
                                     static_cast<ULONG>(std::size(kDisplayName) - 1), path, length,
                                     /*uIconIndex=*/0, kSubstituteLayout, /*dwPreferredLayout=*/0,
                                     /*bEnabledByDefault=*/TRUE, /*dwFlags=*/0);
}

HRESULT unregisterProfile() noexcept {
    ComPtr<ITfInputProcessorProfileMgr> profiles;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&profiles));
    if (FAILED(hr)) return hr;
    hr = profiles->UnregisterProfile(kClsidTextService, kLangIdBanglaBangladesh, kGuidProfile, 0);
    return hr;
}

HRESULT registerCategories() noexcept {
    ComPtr<ITfCategoryMgr> categories;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&categories));
    if (FAILED(hr)) return hr;
    for (const GUID* category : kCategories) {
        hr = categories->RegisterCategory(kClsidTextService, *category, kClsidTextService);
        if (FAILED(hr)) return hr;
    }
    return S_OK;
}

HRESULT unregisterCategories() noexcept {
    ComPtr<ITfCategoryMgr> categories;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&categories));
    if (FAILED(hr)) return hr;
    HRESULT result = S_OK;
    for (const GUID* category : kCategories) {
        hr = categories->UnregisterCategory(kClsidTextService, *category, kClsidTextService);
        if (FAILED(hr) && SUCCEEDED(result)) result = hr;
    }
    return result;
}

} // namespace

HRESULT registerServer() noexcept {
    ComScope com;
    HRESULT hr = registerComServer();
    if (SUCCEEDED(hr)) hr = registerProfile();
    if (SUCCEEDED(hr)) hr = registerCategories();
    if (FAILED(hr)) unregisterServer();  // roll back: never leave a broken registration
    return hr;
}

HRESULT unregisterServer() noexcept {
    ComScope com;
    // Undo everything, reporting the first failure but attempting every step.
    HRESULT result = unregisterCategories();
    const HRESULT profile = unregisterProfile();
    if (SUCCEEDED(result)) result = profile;
    const HRESULT com2 = unregisterComServer();
    if (SUCCEEDED(result)) result = com2;
    return result;
}

} // namespace july::tip
