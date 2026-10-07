# Release process

## Build

```
powershell -ExecutionPolicy Bypass -File tools\Build-Installer.ps1            # preview
powershell -ExecutionPolicy Bypass -File tools\Build-Installer.ps1 -Release   # production
```

`-Release` builds with `STRICT_LAYOUT=ON`, so it **fails** until every layout row and every
Classic glyph row is verified. Before building a release, bump `VERSION` in
`CMakeLists.txt`.

## Production gates

A build is production-ready only when every gate is green. Status as of 0.3.0:

| # | Gate | Status |
|---|---|---|
| 1 | All unit, golden, fuzz and generator tests pass (x64 Debug/Release, x86 Release) | ✅ 22/22 ctest |
| 2 | Static analysis (`/analyze`) clean | ✅ |
| 3 | AddressSanitizer run of unit tests and fuzzer clean | ✅ (last run at 0.2.0, engine unchanged since) |
| 4 | End-to-end TSF smoke test (`july_tip_smoke`) passes against the **installed** build | ✅ 18/18 on 0.3.0 (run by double-clicking `Keyboard-Test.cmd`) |
| 5 | Every Bijoy layout row verified against the master chart | ✅ 79/79 |
| 6 | Every SutonnyMJ Classic row verified in the font (`source=verified`) | ✅ 222/222 (2026-10-07; 15 rows corrected, see the table header) |
| 7 | Manual compatibility checklist (docs/COMPATIBILITY.md) in Notepad, Word, Chrome, Edge, Firefox, WinUI | ⏳ owner reported "working fine" (2026-10-07); per-app results still to record |
| 8 | Install → upgrade → uninstall → reinstall on a clean machine; no leftover CTF/CLSID keys | ⏳ install verified; uninstall not yet |
| 9 | Binaries and installer Authenticode-signed and timestamped | ❌ no certificate yet |
| 10 | Legal: layout/name review done; splash artwork rights and credit confirmed | ⏳ see THIRD_PARTY_NOTICES.md |
| 11 | Spectre-mitigated build (`JULY_SPECTRE=ON`) | ❌ VS component not installed |

## Code signing

Sign the files in this order:

1. `JulyTip.dll` (x64 and x86)
2. `JulyBangla.exe`
3. the installer itself

For Inno Setup, the installer is signed with `SignTool=` in `[Setup]`. Timestamp every
signature so it stays valid after the certificate expires:

```
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a <file>
signtool verify /pa /v <file>
```

An EV or OV code-signing certificate is required. Never tell users to disable SmartScreen
or antivirus.

## Versioning

| Number | Source | Change when |
|---|---|---|
| Application | `VERSION` in CMakeLists.txt, SemVer | every release |
| Engine | `JULY_ENGINE_VERSION` | composer or backend behaviour changes |
| Layout | `JULY_LAYOUT_VERSION` | any change to `layouts/*.layout` or `*.classic` |

## Android gates

| # | Gate | Status |
|---|---|---|
| A1 | Debug/release builds and JVM unit tests pass | ✅ |
| A2 | Tested on a real phone | ✅ owner reported "working fine" (2026-10-07); device/apps to record |
| A3 | Final package name chosen (cannot change after the first Play release) | ⏳ `org.julybangla.keyboard` |
| A4 | Release signing key created and backed up; App Bundle signed | ❌ |
| A5 | Accessibility (TalkBack) for the on-screen keys | ❌ |
| A6 | Play Console listing, privacy policy ("collects no data"), data-safety form | ❌ |
