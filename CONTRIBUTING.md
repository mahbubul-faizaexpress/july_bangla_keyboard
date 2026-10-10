# Contributing

Thank you for helping. Bug reports, typing-rule corrections, translations and code are
all welcome. You can write in Bangla or English.

## Reporting a bug

Open an issue with the **Bug report** template. Include:

- the platform and version (Windows 11 23H2, or Android 14 on a Pixel 7, for example)
- the app you were typing in (Word, Chrome, WhatsApp…)
- the mode (বাংলা / ক্লাসিক / English)
- the **exact keys** you pressed, what you expected, and what appeared

For typing bugs, the key sequence matters most. Write it the way the golden tests do,
with space-separated US keys where an uppercase letter means Shift: `j m A` → কর্ম.

## Layout and Classic table changes

`layouts/bijoy.layout` and `layouts/sutonnymj.classic` are the source of truth.

- **Layout:** every row needs a `source`. Release builds reject unconfirmed (`memory`)
  rows. Cite the chart you checked.
- **Classic table:** a row is `verified` only after it was rendered in the SutonnyMJ font.
  Never commit font files.
- Add or extend a golden test in `tests/golden/` for every behaviour you change.

## Code changes

1. Fork the repository and create a branch from `main`.
2. Build and run the tests for what you touched:
   - **Windows:** `cmake --preset x64 && cmake --build --preset x64-release && ctest --preset x64-release`
   - **Android:** `cd android && gradlew assembleDebug testDebugUnitTest`
3. Keep to the existing style:
   - C++20 with no exceptions in the engine and no allocation per key.
   - Warnings are errors (`/W4 /WX`, `-Wall -Wextra -Werror`).
4. Open a pull request using the template, and explain what changed and how you tested it.

## Ground rules

- **No telemetry or network access, and no keylogging**, in any form.
- **No global keyboard hooks, no injected keystrokes**, and no attempts to get around
  security software or anti-cheat systems.
- **The engine stays platform-independent.** Platform code belongs in `src/tip`,
  `src/app` or `android/`.
- **Every change to engine behaviour comes with a test.**

By contributing, you agree that your contribution is licensed under the project's license,
the GNU General Public License version 3 or later (see [LICENSE](LICENSE)).

Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).
