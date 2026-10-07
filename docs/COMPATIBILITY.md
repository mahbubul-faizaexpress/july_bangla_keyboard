# Compatibility matrix

Version 0.2.0. Only results that were actually observed are recorded. A blank cell means
"not tested yet".

Legend: ✅ works · ❌ fails (see notes) · ⚠️ works with a caveat · — not applicable

## How each result was obtained

- **Automated:** `july_tip_smoke.exe` (built with the tests) drives the registered text
  service through real TSF. It uses a TSF-enabled RichEdit control and delivers keys via
  `ITfKeystrokeMgr`. 18/18 checks pass on 0.3.0 (2026-10-07):
  - 15 typing cases, including numpad digits and a mouse click that moves the caret
    out of an open syllable
  - 3 input-indicator checks
- **User:** typed by hand on the developer machine (Windows 11 build 26300).

## Results

| Application / control | Unicode | Classic | Composition | Backspace | Enter/Space after syllable | Mode switch | Source |
|---|---|---|---|---|---|---|---|
| RichEdit with TSF (Notepad's control type) | ✅ | ✅ | ✅ | ✅ | ✅ text final before the key reaches the app | ✅ | automated |
| VS Code (Electron) chat input | ✅ | | ✅ | | ✅ after fix `c479c3b` (restart required) | ✅ | user |
| Notepad (Windows 11) | | | | | | | |
| Microsoft Word | | | | | | | |
| Excel cell | | | | | | | |
| Chrome: Google search box | | | | | | | |
| Chrome: Facebook post box (contenteditable) | | | | | | | |
| Edge: address bar | | | | | | | |
| Firefox: textarea | | | | | | | |
| Windows Settings search (WinUI) | | | | | | | |
| Start menu search | | | | | | | |
| Run dialog, Win+R (classic EDIT via CUAS) | | | | | | | |
| Adobe Photoshop / Illustrator | | | | | | | |
| Elevated app (e.g. Notepad as admin) | | | | | | | |

**Bugs found by the user, fixed, and covered by regression tests:**

1. অ + া stayed as two letters ("অামি") instead of আ. Fixed in `c479c3b`.
2. In VS Code, the last syllable was lost when Enter was pressed ("বাংলাদে"). Fixed in
   `c479c3b`.

## Manual test checklist (per application)

1. Select **July Bangla Keyboard** with Win+Space.
2. Press **Ctrl+Alt+B** and check that the status bar, the tray icon and the Windows input
   indicator all show **বাংলা**.
3. Type `g f d m` → **আমি**, `j g Shift+N` → **ক্ষ**, `j m Shift+A` → **কর্ম**,
   `c j f` → **কো**, `Shift+F f d m` → **আমি**.
4. Type `j g Shift+N`, press **Backspace** → **ক্**.
5. Type a word, then press **Enter**: the whole word must stay, including the last letter.
6. Select some text and type: the selection is replaced.
7. Click elsewhere in the middle of a word: nothing is lost or duplicated.
8. Press **Ctrl+Alt+B** to switch to বিজয় and type `g f d m` → **Avwg**. It shows Bangla
   only with the SutonnyMJ font.
9. Press **Ctrl+Alt+B** to switch to EN and type `test` → **test**.
10. Paste (Ctrl+V) and undo (Ctrl+Z) behave normally.

## Known limitations (by design)

- **Running programs keep the text-service version they loaded.** Restart them, or sign
  out and back in, after an update.
- **Protected games and anti-cheat software** may refuse third-party input methods. This
  product never tries to bypass them.
- **The secure desktop (UAC prompt, logon screen)** is not supported.
- **Classic text displays as Bangla only in the SutonnyMJ font.** In non-Unicode (ANSI)
  programs it also needs system code page 1252.
