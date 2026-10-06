# Roadmap, testing strategy, risks

## Testing strategy

| Layer | What | How |
|---|---|---|
| Unit | Token table, FSM transitions, rule trie, backends, `calculateEditDelta`, ring buffer | In-repo minimal test harness (`tests/JulyTest.h`, no third-party code), runs in < 1 s |
| Golden vectors | Every key, every consonant/vowel/vowel sign/digit/punctuation, every linker sequence, every Classic conjunct | `tests/golden/*.tsv`: `keys<TAB>expected Unicode code points<TAB>expected Classic code units`, compared by exact code point, never visually |
| Layout validation | Generator rejects duplicate keys, unknown tokens, unreachable rules, and unverified Classic rows in release mode | `layoutgen --validate` in CI |
| Unicode | NFC/NFD consistency of every emitted syllable, ZWJ/ZWNJ cases, no surrogate splitting, mixed English/Bangla, emoji adjacency (engine never edits outside its composition) | unit + Windows `NormalizeString` cross-check |
| Fuzz | Random keys, modifiers, backspace, navigation, mode switches, focus resets | Deterministic seeded fuzzer (and libFuzzer via clang-cl when available). Asserts: no crash, bounded state, output equals a replay of the same seed, every op is O(1) |
| Regression | Every fixed bug becomes a golden or fuzz-seed case | Mandatory in review |
| TSF integration | TIP inside a test host using a real `ITfThreadMgr` and a minimal text store | Automated; manual for real apps |
| App compatibility | Matrix in COMPATIBILITY.md (Unicode, Classic, composition, backspace, cursor, selection replace, paste, mode switch, focus switch) | Manual checklist per release, at 100/125/150/200 % DPI |
| Performance | `keyboard_benchmark.exe` at 10⁴/10⁵/10⁶ events; idle and active CPU and memory via WPR/WPA and Process Explorer | Reported numbers only, never estimated |

## Phases and gates

Each phase ends with: compile (zero warnings at `/W4 /WX`), all tests green, relevant
benchmark run, docs updated, and a logical git commit.

| Phase | Deliverable | Gate |
|---|---|---|
| 0 | Architecture docs (this) | Architecture reviewed |
| 1 | CMake + presets, engine static lib skeleton, doctest, empty TIP DLL/companion/benchmark targets, CI script | x64 + x86 builds clean |
| 2 | `.layout` format, `layoutgen`, Bijoy key → token tables, full key golden tests | Every key mapped and tested **against a cited Bijoy reference** |
| 3 | Syllable FSM, rule trie, Unicode backend, backspace-in-composition, ring buffer, fuzzer | All golden vectors pass, fuzz 10⁷ events clean |
| 4 | Classic backend, SutonnyMJ tables, cp1252 mapping, ACP diagnostics | Classic golden vectors pass; rows verified with a real font |
| 5 | TIP: COM server, registration, `ITfTextInputProcessorEx`, key sink, edit session, composition, focus/edit/composition sinks, compartment, preserved key, input-mode indicator. **Includes the spikes for the [VERIFY] items.** | Types correctly in Notepad, Word, Edge; registration and unregistration are clean |
| 6 | Mode switching end-to-end, settings read at activation | Mode consistent across apps |
| 7 | Companion: tray, status bar, settings, font check, startup | Idle CPU 0, memory measured |
| 8 | Compatibility matrix execution and fixes; fallback decision | Matrix filled with real results |
| 9 | Integration tests, regression suite hardening | Green in CI |
| 10 | Benchmarks, profiling, optimisation | Report with measured numbers |
| 11 | WiX MSI (x64 + x86 TIP), registration custom actions with rollback, upgrade, uninstall | Install/upgrade/uninstall/rollback leave no stale CTF/CLSID keys |
| 12 | Signing, release docs, reproducibility, SECURITY review | Signed release verified with `signtool verify /pa` |

## Major risks and mitigations

| Risk | Mitigation |
|---|---|
| Bijoy layout details (exact key positions, rare conjunct rules) recalled incorrectly | Layout data cites a reference source per row. Golden tests are reviewed by a native Bijoy typist before Phase 2 closes. |
| SutonnyMJ glyph table errors | Each Classic row is marked `verified` only after rendering in SutonnyMJ; unverified rows block release |
| Global compartment does not reach AppContainer or elevated processes | Phase 5 spike; fallback to per-thread compartment seeded from HKCU |
| Composition underline or composition events disturb some web editors (contenteditable frameworks) | Display-attribute provider (no underline); short compositions committed at syllable end; tested in Phase 8 |
| Apps that terminate compositions unexpectedly | `OnCompositionTerminated` → reset the engine without touching text |
| Async edit sessions reorder operations | Each session carries its own immutable op; ops are applied FIFO; covered by integration tests |
| Classic in ANSI apps on a non-1252 ACP | Diagnostic warning; documented limitation |
| A crash in the TIP takes down the host app | No exceptions across COM, fuzzing, ASan builds, Application Verifier runs, conservative fail-safe (on error, end composition and pass keys through) |
| Per-machine install needs admin (conflicts with "prefer per-user") | Unavoidable for TSF; elevation only at install time, documented |
| 32-bit host apps | Ship an x86 TIP DLL |
| AV false positives | Documented APIs only, no hooks, signed binaries, no packing, MSI install |
| **Toolchain not installed on the dev machine** | Install VS Build Tools + Windows SDK + CMake before Phase 1 (see below) |

## Prerequisites for Phase 1 (not yet installed on this machine)

- Visual Studio 2026 (or 2022) Build Tools with these workloads/components:
  "Desktop development with C++", MSVC x64/x86 tools, Windows 11 SDK, and C++ CMake tools
  (provides CMake and Ninja).
  For example: `winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"`
- Later phases: WiX Toolset (Phase 11) and the Windows SDK `signtool` (Phase 12).
- For testing Classic mode: a legitimately licensed SutonnyMJ font.

## Definition of done

As in spec §72: Unicode and Classic typing, composition, backspace, cursor-safety, mode
switching, TSF registration, common apps, app switching, no input loss under rapid typing,
fuzz-clean, measured CPU/memory, working install/uninstall/registration cleanup,
documentation, signed release, and documented security model. **None of these are claimed
until verified.**
