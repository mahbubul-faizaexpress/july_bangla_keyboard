# Microsoft Store submission

The keyboard is a TSF text service, so it is submitted as an **EXE app** (not MSIX).
Microsoft does not sign EXE installers: the installer and every binary inside it must be
signed with a certificate from a CA in the Microsoft Trusted Root Program before
submission. See [RELEASE.md](RELEASE.md) for the gates.

Sources: [MSI/EXE package requirements](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements),
[IME requirements](https://learn.microsoft.com/en-us/windows/apps/develop/input/input-method-editor-requirements).

## Checklist

| # | Item | Status |
|---|---|---|
| 1 | Open-source licence (needed for free signing) | ✅ GPL-3.0-or-later |
| 2 | Automated release build (`.github/workflows/release.yml`) | ✅ |
| 3 | Installer and all PE files code-signed | ❌ no certificate yet |
| 4 | Silent install tested (`/VERYSILENT /SUPPRESSMSGBOXES /NORESTART`) | ⏳ supported by Inno Setup; not yet run |
| 5 | Offline, standalone installer | ✅ |
| 6 | Versioned HTTPS download URL that never changes | ✅ `…/releases/download/v<version>/JulyBanglaKeyboard-Setup.exe` |
| 7 | Privacy policy URL | ✅ `<website>/privacy.html` |
| 8 | Uninstall → reinstall leaves nothing behind | ⏳ |
| 9 | IME categories (`IMMERSIVESUPPORT`, `SYSTRAYSUPPORT`), icons in the DLL, `RegisterProfile` | ✅ |
| 10 | Store logo (`resources/store/logo-300.png`) and at least one screenshot | ⏳ logo done; screenshots to take |
| 11 | Partner Center account and reserved name | ⏳ owner |

Possible certification questions, from the IME guidelines:

- **Always-visible window.** The status bar is shown by default and the guidelines
  discourage IME windows that are always visible. If it is flagged, make it off by default.
- **Colour icon.** The guidelines ask for black-and-white IME icons. If it is flagged, add
  a monochrome branding icon.

## Partner Center fields

| Field | Value |
|---|---|
| App type | EXE or MSI app |
| Package URL | `https://github.com/<owner>/july_bangla_keyboard/releases/download/v<version>/JulyBanglaKeyboard-Setup.exe` |
| Architecture | x64 |
| Installer parameters | `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART` |
| App language | Bangla (Bangladesh), English |
| Category | Utilities & tools |
| Price | Free |
| Privacy policy | `<website>/privacy.html` |
| Website | `<website>/` |
| Support contact | `<website>/guide.html` |

A silent install selects both installer tasks: the keyboard is added to the user's
keyboard list and the status bar starts with Windows.

## Listing text: English

**Name:** July Bangla Keyboard

**Short description:**
Type Bangla with the familiar Bijoy-compatible layout, in any app. Free, open source,
and completely offline.

**Description:**

July Bangla Keyboard lets you type Bangla the way you already know: the Bijoy-compatible
key layout and typing rules, in every Windows app.

Three modes, one shortcut. Press Ctrl+Alt+B to switch between:

- বাংলা: Unicode Bangla for the web, email, Facebook and Office
- ক্লাসিক: SutonnyMJ (ANSI) output for legacy documents and print work
- English

A small bar at the top of the screen and a tray icon always show the current mode.

Private by design. The keyboard never connects to the internet. It does not collect,
store or send anything you type. There are no ads and no accounts.

Built the right way. It uses the Windows Text Services Framework, so it works in ordinary
apps such as Word, Notepad, Chrome and Edge. It does not use keyboard hooks.

Light and fast. The installer is under 3 MB.

Named in remembrance of the July 2024 student–people's uprising in Bangladesh.

"Bijoy" is a trademark of its owner. July Bangla Keyboard is an independent project and
is not affiliated with Bijoy. The SutonnyMJ font is not included.

**Features (one per line):**

- Bijoy-compatible layout and typing rules
- Unicode and Classic (SutonnyMJ) output
- Ctrl+Alt+B switches English, বাংলা and ক্লাসিক
- Mode bar and tray icon
- Works offline; collects no data
- Free and open source (GPL-3.0)

**Keywords:** Bangla keyboard, Bengali keyboard, Bijoy, Unicode Bangla, SutonnyMJ,
বাংলা কীবোর্ড, Bangla typing

## Listing text: বাংলা

**নাম:** জুলাই বাংলা কীবোর্ড

**সংক্ষিপ্ত বিবরণ:**
পরিচিত বিজয়-সামঞ্জস্যপূর্ণ লেআউটে যেকোনো অ্যাপে বাংলা লিখুন। বিনামূল্যে, ওপেন সোর্স, ইন্টারনেট
ছাড়াই।

**বিবরণ:**

জুলাই বাংলা কীবোর্ড দিয়ে আপনি চেনা নিয়মেই বাংলা লিখবেন: বিজয়-সামঞ্জস্যপূর্ণ কী-বিন্যাস আর লেখার
নিয়ম, Windows-এর সব অ্যাপে।

তিনটি মোড, একটি শর্টকাট। Ctrl+Alt+B চাপলে মোড বদলায়:

- বাংলা: ওয়েব, ইমেইল, Facebook ও Office-এর জন্য ইউনিকোড বাংলা
- ক্লাসিক: পুরনো দলিল ও ছাপার কাজের জন্য SutonnyMJ (ANSI) লেখা
- English

স্ক্রিনের উপরের ছোট বার আর ট্রে-আইকন সবসময় দেখায় এখন কোন মোড চলছে।

আপনার লেখা আপনারই। কীবোর্ড কখনো ইন্টারনেটে যুক্ত হয় না। আপনি যা লেখেন তা সংগ্রহ, সংরক্ষণ বা
কোথাও পাঠানো হয় না। কোনো বিজ্ঞাপন বা অ্যাকাউন্ট নেই।

সঠিকভাবে তৈরি। Windows-এর নিজস্ব ইনপুট ব্যবস্থা ব্যবহার করে, তাই Word, Notepad, Chrome,
Edge-এর মতো সাধারণ অ্যাপে কাজ করে। কোনো কীবোর্ড-হুক নেই।

হালকা ও দ্রুত। ইনস্টলার ৩ MB-এরও কম।

২০২৪ সালের জুলাইয়ের ছাত্র-জনতার গণঅভ্যুত্থানের স্মরণে নাম রাখা।

“বিজয়” তার স্বত্বাধিকারীর ট্রেডমার্ক। জুলাই বাংলা কীবোর্ড একটি স্বাধীন প্রকল্প, বিজয়ের সাথে
সম্পর্কিত নয়। SutonnyMJ ফন্ট এর সাথে দেওয়া হয় না।

## Screenshots to take

Partner Center states the required count and sizes when you upload.

1. Typing Bangla in Word or Notepad, with the mode bar showing "বাংলা".
2. The tray menu with the three modes.
3. Classic mode with text shown in SutonnyMJ.
4. The splash screen.
