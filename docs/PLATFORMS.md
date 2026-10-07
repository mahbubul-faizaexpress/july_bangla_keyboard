# Platforms plan: Windows, Android, Linux, macOS

One repository and **one engine**: `src/engine/` (C++20, no OS dependencies). It holds the
Bijoy layout tables, the composer (visual-order kar, reph, linker, অ + া = আ), the Unicode
and Classic backends, and their tests. Each platform adds a thin layer that delivers keys
to the engine and writes the engine's `commit` and `composition` text through that
platform's input-method API.

```
src/engine/      shared engine (all platforms)
layouts/         shared Bijoy layout and verified SutonnyMJ table
tests/           shared golden files and engine tests
src/tip, src/app Windows (TSF text service, status bar, tray)
android/         Android (InputMethodService, Kotlin + JNI bridge to the engine)
linux/           Linux (Fcitx5 addon, later IBus)                     -- planned
macos/           macOS (Input Method Kit)                              -- planned
```

## Mapping the engine to each platform

The engine's `EditResult` already matches the composition model each platform uses:

| Engine | Windows (TSF) | Android | Linux Fcitx5 | Linux IBus | macOS IMK |
|---|---|---|---|---|---|
| `composition` | composition range `SetText` | `InputConnection.setComposingText` | `InputPanel` preedit | `update_preedit_text` | `setMarkedText` |
| `commit` | end composition | `commitText` | `commitString` | `commit_text` | `insertText` |
| key event | `ITfKeyEventSink` | on-screen key / `onKeyDown` | `keyEvent` | `process_key_event` | `handleEvent` |
| mode switch | Ctrl+Alt+B (preserved key) | key on the keyboard | hotkey | hotkey | hotkey / menu |

The engine identifies keys by PC scan code. On-screen keyboards pass the scan code of the
Bijoy key they draw. Hardware keyboards map the platform key code to the scan code with
a small table.

## Android (next)

- **Language:** Kotlin for `InputMethodService` and the on-screen keyboard (an Android
  requirement), plus a small C++ JNI bridge to the shared engine, built with the NDK and
  CMake.
- **UI:** an on-screen Bijoy-style layout drawn with a plain Android `View`, with no
  heavy UI framework. Then long-press alternatives, themes, and a key sound/vibration
  setting.
- **Modes:** Unicode first. Classic (SutonnyMJ) is rarely useful on phones and can come
  later.
- **Hardware keyboards** (tablets, Chromebooks): full Bijoy typing through the same
  engine.
- **Not used:**
  - Flutter or React Native, which are poor fits for an input-method service.
  - A second engine written in Kotlin, which would drift from the Windows behaviour.
  - Code from GPL keyboards (for example HeliBoard), which would force the project's
    licence. Apache-2.0 FlorisBoard is fine to learn from.
- **Tools needed:** JDK 17, Android SDK command-line tools (platform, build tools, NDK,
  CMake), and Gradle (wrapper). About 3–4 GB in total. A phone with USB debugging, or the
  Android emulator, is needed for testing.
- **Publishing:** Google Play Console (one-time USD 25). The APK is expected to be about
  2–3 MB.

## Linux

- **Framework:** Fcitx5 first. Its addons are C++, so the engine links directly, and it is
  the default input framework on KDE and many distributions. IBus (Ubuntu/GNOME default,
  C/GObject) comes second as a thin wrapper around the same engine.
- **Mode indicator:** the framework's own status icon. No separate tray program is needed.
- **Packaging:** `.deb` (Ubuntu/Debian), later `.rpm`, Flatpak, and AUR.
- **Classic mode:** works unchanged. The output is ordinary text that displays in
  SutonnyMJ.
- **Effort:** small. Only the key and text bridge and the packaging are new.

## macOS

- **Framework:** Input Method Kit (`IMKInputController`), written in Objective-C++/Swift
  around the same C++ engine. Installed as an input-source bundle in
  `/Library/Input Methods`.
- **Requirements:**
  - A Mac to build and test.
  - An Apple Developer account (USD 99/year) to sign and notarize, without which macOS
    will not run a third-party input method for other users.
- **Mode indicator:** the input-source menu in the menu bar.
- **Effort:** medium.

## Order

1. Windows 1.0 release (open source).
2. Android.
3. Linux (Fcitx5, then IBus).
4. macOS.

At every step the shared golden tests must pass on that platform, so all versions type
the same text for the same keys.
