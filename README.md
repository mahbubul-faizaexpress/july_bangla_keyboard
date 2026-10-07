# জুলাই বাংলা কীবোর্ড (July Bangla Keyboard)

> জুলাইয়ের অনুভূতি স্মরণ হোক, বাংলা লেখার মাধ্যমে।
>
> ২০২৪ সালের জুলাইয়ের ছাত্র-জনতার গণঅভ্যুত্থানের স্মরণে

A native Windows Bangla input method written in C++20, with Bijoy-style typing in three
modes:

- **English**
- **বাংলা** — Unicode output
- **বিজয়** — Classic output (SutonnyMJ / Bijoy ANSI compatible)

`Ctrl+Alt+B` cycles through the modes.

## Design

The keyboard is a Windows **Text Services Framework (TSF)** text service. It has:

- no global keyboard hooks
- no injected keystrokes
- no runtime dependencies
- no network access

A small companion program shows the current mode in a Bijoy-style floating bar, a tray
icon and Windows' own input indicator.

## Status

Version 0.2.0 (preview). What works:

- Typing in Unicode and Classic modes, verified end to end through TSF.
- The mode switch, status bar, tray icon and splash screen.
- An Inno Setup installer.

Not done yet:

- The Classic (SutonnyMJ) glyph table has not been verified in the SutonnyMJ font.
- The application compatibility matrix is still being filled.
- The binaries are not code-signed yet.

## Build

Visual Studio 2026 Build Tools (MSVC x64/x86, Windows 11 SDK, CMake). See
[docs/BUILD.md](docs/BUILD.md).

```
cmake --preset x64 && cmake --build --preset x64-release && ctest --preset x64-release
powershell -ExecutionPolicy Bypass -File tools\Build-Installer.ps1
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Keyboard layout](docs/KEYBOARD_LAYOUT.md)
- [Classic mode](docs/CLASSIC_MODE.md)
- [Compatibility](docs/COMPATIBILITY.md)
- [Performance](docs/PERFORMANCE.md)
- [Security and privacy](docs/SECURITY.md)
- [Roadmap](docs/ROADMAP.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

## License

Copyright © 2026 Mahbubul Alam. All rights reserved. No license is granted at this time.

`layouts/sutonnymj.classic` is under MPL-2.0; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
