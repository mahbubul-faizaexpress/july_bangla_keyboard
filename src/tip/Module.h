#pragma once

#include <windows.h>

#include <atomic>

namespace july::tip {

// DLL-wide state for COM lifetime (DllCanUnloadNow).
struct Module {
    static inline HINSTANCE instance = nullptr;
    static inline std::atomic<long> objects{0};
    static inline std::atomic<long> locks{0};
};

// RAII object counter: every COM object of the DLL holds one.
struct ObjectCount {
    ObjectCount() noexcept { ++Module::objects; }
    ~ObjectCount() { --Module::objects; }
    ObjectCount(const ObjectCount&) = delete;
    ObjectCount& operator=(const ObjectCount&) = delete;
};

HRESULT registerServer() noexcept;
HRESULT unregisterServer() noexcept;

} // namespace july::tip
