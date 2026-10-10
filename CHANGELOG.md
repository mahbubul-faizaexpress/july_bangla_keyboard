# Changelog

All notable changes are listed here. Versions follow [SemVer](https://semver.org/).

## [Unreleased]

## [0.4.0] - 2026-10-10

### Licence
- The project is now licensed under the **GNU GPL, version 3 or later**.

### Added
- **Android preview.** An iPhone-style on-screen keyboard (four rows, ১২৩ and #+= pages, key
  preview, action labels on Return) and hardware-key support. It shares the C++ engine
  over JNI and requests no permissions.
- **Logo** on Windows (program, text service, installer) and Android.
- **Brand colours.**
  - Logo blue is the interface colour.
  - July red is used for the slogan and Classic mode.
- **Remembrance splash on Android.** It stays at least 3.5 seconds and fades in and out.
- **Project website** (`website/`) with these pages: home, download (detects the device),
  user guide, about, privacy, terms.
- **Repository files:**
  - CONTRIBUTING, Code of Conduct, security policy
  - issue and pull request templates
  - CI workflow
- **Automated releases.** Pushing a version tag builds, tests and publishes the installer
  (`.github/workflows/release.yml`).
- **Microsoft Store** submission checklist and listing text (`docs/STORE.md`).

### Changed
- **Windows splash.**
  - It no longer takes the keyboard focus.
  - It stays 5 seconds; clicks are ignored for the first 3.5 seconds.

## [0.3.0] - 2026-10-07

### Added
- All 222 SutonnyMJ Classic rows verified in the font, with 15 corrected. `STRICT_LAYOUT`
  is now on by default.
- A text-edit sink. Committing a syllable never rewrites text or moves the caret.
- Release gates and Bangla troubleshooting docs.

### Changed
- The Classic mode is now named “ক্লাসিক”.
- Splash polish. `/analyze` findings fixed.

## [0.2.0] - 2026-10-06

### Added
- Inno Setup installer, version resources, and the Windows input-indicator button.

## [0.1.0] - 2026-10-06

### Added
- First working TSF text service with the Unicode and Classic backends, the companion
  status bar and the tray icon.
