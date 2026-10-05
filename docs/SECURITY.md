# Security and privacy

## Privacy statement

July Bangla Keyboard works entirely offline. It contains **no network code**. It does
not record, store, or transmit keystrokes, typed text, or clipboard contents. It has no
telemetry or analytics.

The engine keeps only the keys of the syllable currently being composed, plus a small
in-memory history used for composition (at most 64 logical tokens per UI thread). This
history is cleared on every focus change and never written to disk.

## Why the software behaves legitimately

| Practice | How it is followed |
|---|---|
| Documented input architecture | It is a Text Services Framework input method, the mechanism Windows provides for keyboards and IMEs. TSF loads the DLL only when the user selects the input method. |
| No keystroke interception | No `SetWindowsHookEx` keyboard hooks, no raw-input sinks, no `SendInput` in the core product |
| No code injection | No `CreateRemoteThread`, `WriteProcessMemory`, AppInit_DLLs, or similar. Loading is done by Windows TSF according to registered profiles. |
| No evasion | No packing, obfuscation, self-modifying code, process hiding, or anti-cheat/AV interaction. Protected applications that refuse input methods are a documented limitation. |
| Transparent persistence | MSI install listed in Apps & Features. The optional startup entry is a visible `HKCU\...\Run` value. Uninstall removes all COM and TSF registrations. |
| Signed binaries | All binaries and the MSI are Authenticode-signed and timestamped (from Phase 12) |
| Least privilege | Admin is needed only by the installer to register the TSF profile. Runtime runs at the user's integrity level. |

## Diagnostics

Diagnostic logging is off by default. When enabled, it records only lifecycle events,
HRESULT/Win32 error codes, registration results, and timing metrics. It never records key
codes, characters, typed text, passwords, or clipboard data.

The diagnostic test application shows test keystrokes on screen for local debugging only.
It does not persist or transmit them.

## Reporting vulnerabilities

To be defined before the first public release.
