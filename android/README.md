# July Bangla Keyboard for Android (planned)

The Android keyboard will live here. It is a Kotlin `InputMethodService` with a Bijoy-style
on-screen layout, plus a small JNI bridge to the shared C++ engine in `../src/engine`.
The engine, layouts and golden tests are shared with Windows, so the same keys give the
same text on both platforms.

See [docs/PLATFORMS.md](../docs/PLATFORMS.md) for the plan and the tools needed to build.
