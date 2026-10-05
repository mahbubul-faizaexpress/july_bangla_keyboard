# July Bangla Keyboard

A native Windows Bangla input method written in C++20. It supports Bijoy-style typing in
three modes:

- **English**
- **Bangla Unicode**
- **Bangla Classic** (Bijoy ANSI / SutonnyMJ-compatible)

`Ctrl+Alt+B` cycles through the modes.

It is implemented as a Text Services Framework (TSF) text input processor. It uses no
global keyboard hooks, no runtime dependencies, and no network access.

**Status:** Phase 0 (architecture). Nothing is buildable yet.

- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap, testing and risks](docs/ROADMAP.md)
- [Security and privacy](docs/SECURITY.md)
