# July Bangla Keyboard for Android (preview)

An Android keyboard (`InputMethodService`) with an on-screen Bijoy layout. It uses the
same C++ engine as Windows (`../src/engine`, through a small JNI bridge), so the same keys
give the same text on both platforms.

- **Modes:** বাংলা (Unicode), ক্লাসিক (SutonnyMJ), English. Tap the mode key to cycle;
  long-press it to switch to another keyboard. Ctrl+Alt+B cycles on hardware keyboards.
- **Look:** like the iPhone keyboard. It has four rows, white letter keys with a soft
  shadow on a grey tray, and grey function keys. A pressed letter shows an enlarged
  preview, and Return names the field's action (যাও, খুঁজুন, পাঠান…) and turns blue.
  Digits and signs are on the ১২৩ and #+= pages, and a space returns to the letters.
  Light and dark themes follow the phone.
- **Layout:** the PC Bijoy key positions. The middle row ends with ৎ/ঃ (the `\` key),
  and ঁ (Bijoy Shift+7) sits beside m. Each key shows its Shift character in the corner.
  The linker `g`, reph and pre-base kars type exactly as on Windows.
- **Number, phone and password fields** always get plain Latin characters.
- **Privacy:** no permissions at all, no network, nothing typed is stored. The only
  saved setting is the mode.
- **Size:** the release APK is about 0.4 MB for both CPU types. A Play Store App Bundle
  is smaller per phone.

## Layout

```
android/
  app/src/main/cpp/          CMakeLists.txt (builds src/engine + JulyJni.cpp), JNI bridge
  app/src/main/java/...      JulyImeService (typing), KeyboardView (drawing), Keys,
                             Engine (JNI wrapper), SetupActivity (enable / choose)
  app/src/main/res/          strings (Bangla), colors (light/dark), IME metadata, icon
  app/src/test/...           JVM unit tests (scan codes, page widths, result decoding)
```

## Build

Requirements:

- JDK 17
- Android SDK with `platforms;android-36`, `ndk;28.2.13676358` and `cmake;3.31.6`
- a host C++ compiler and CMake, used to run the table generators in `tools/layoutgen`
  on the build machine. On Windows the CMake bundled with Visual Studio is found
  automatically. Elsewhere `cmake` must be on PATH, or set `july.hostCmake` in
  `local.properties`.

```
cd android
gradlew assembleDebug testDebugUnitTest     # app/build/outputs/apk/debug/app-debug.apk
gradlew assembleRelease                     # shrunk release APK (unsigned)
gradlew bundleRelease                       # App Bundle for Google Play (sign before upload)
```

`local.properties` (not committed) holds `sdk.dir=...`.

## Try it on a phone

1. On the phone, turn on Developer options and USB debugging.
2. Connect the phone by USB and run `adb install -r app/build/outputs/apk/debug/app-debug.apk`.
   Alternatively, copy the APK to the phone and open it.
3. Open "জুলাই বাংলা কীবোর্ড". Tap ১ to enable the keyboard and ২ to choose it, then type
   in the box.

## Not done yet

- Long-press alternatives, key preview popup, sound and vibration settings, themes.
- Accessibility (TalkBack) for the on-screen keys.
- Release signing and the Play Store listing.
- The final launcher icon (the July logo) to replace the placeholder.

## Release signing

The release APK or App Bundle is signed with a key kept **outside** the repository.
Create the key once, then back it up safely: losing it means you can no longer update
the app.

```
keytool -genkeypair -keystore %USERPROFILE%\.julybangla\android-release.jks -storetype PKCS12 ^
  -alias julybangla -keyalg RSA -keysize 4096 -validity 10000
```

Next to the key, create `%USERPROFILE%\.julybangla\android-release.properties`:

```
storeFile=C:/Users/<you>/.julybangla/android-release.jks
storePassword=...
keyAlias=julybangla
keyPassword=...
```

Then add `july.signing=C:/Users/<you>/.julybangla/android-release.properties` to
`android/local.properties`, and run `gradlew assembleRelease` (or `bundleRelease` for
Google Play). Without this file, release builds are unsigned.
