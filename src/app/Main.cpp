// Companion process (tray icon, status bar, settings).
// Phase 1 skeleton: enforces single instance and exits. The event-driven UI arrives in Phase 7.

#include <windows.h>

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {
    HANDLE instanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\JulyBanglaKeyboard.Companion");
    if (instanceMutex == nullptr) return static_cast<int>(GetLastError());
    const bool alreadyRunning = GetLastError() == ERROR_ALREADY_EXISTS;

    CloseHandle(instanceMutex);
    return alreadyRunning ? 1 : 0;
}
