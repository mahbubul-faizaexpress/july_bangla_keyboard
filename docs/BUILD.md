# Building

## Prerequisites

To install the Visual Studio 2026 Build Tools with the C++ workload, recommended
components (MSVC x64/x86 tools and the Windows 11 SDK), and C++ CMake tools, run:

```
winget install --id Microsoft.VisualStudio.BuildTools --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended"
```

Run the commands below from a **Developer PowerShell / Developer Command Prompt for VS**,
or call the CMake bundled with Visual Studio by its full path.

## Configure, build, test

```
cmake --preset x64
cmake --build --preset x64-debug
ctest --preset x64-debug

cmake --build --preset x64-release
ctest --preset x64-release

cmake --preset x86
cmake --build --preset x86-release      # 32-bit TIP DLL for 32-bit host apps
ctest --preset x86-release
```

Output goes to `build/<preset>/`.

| Option | Default | Meaning |
|---|---|---|
| `BUILD_TESTS` | ON | Unit tests (`july_unit_tests`) |
| `BUILD_BENCHMARKS` | ON (x64) | `keyboard_benchmark.exe` |
| `BUILD_LAYOUT_GENERATOR` | ON | Layout table generator (Phase 2) |
| `BUILD_COMPANION` | ON (x64) | Tray/status companion `JulyBangla.exe` |
| `ENABLE_DIAGNOSTICS` | OFF | Privacy-safe diagnostic instrumentation |
| `JULY_SPECTRE` | OFF | Adds `/Qspectre`. It needs the VS component "MSVC Spectre-mitigated libs" (`Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre`) and is **required for release builds**. |

All targets compile with `/W4 /WX /permissive- /sdl /guard:cf` and use the static CRT
(`/MT`), so no VC++ redistributable is required.
