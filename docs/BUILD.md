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

`ctest` runs the unit tests, the layoutgen validation tests, and `july_fuzz` with three
seeds (1,000,000 events each). Two other tools are run by hand:

```
build/x64/bench/Release/keyboard_benchmark.exe report.txt   # engine latency/allocations
build/x64/tests/Release/july_fuzz.exe <seed> <events>        # longer fuzz runs
```

AddressSanitizer build (from Git Bash, `MSYS_NO_PATHCONV=1` stops `/fsanitize` being
rewritten as a path):

```
MSYS_NO_PATHCONV=1 cmake -S . -B build/asan -G "Visual Studio 18 2026" -A x64 \
    -DBUILD_COMPANION=OFF -DBUILD_BENCHMARKS=OFF "-DCMAKE_CXX_FLAGS=/fsanitize=address /EHsc"
cmake --build build/asan --config Debug --target july_unit_tests july_fuzz
# put the MSVC bin\Hostx64\x64 directory (clang_rt.asan_dynamic) on PATH, then run both
```

| Option | Default | Meaning |
|---|---|---|
| `BUILD_TESTS` | ON | Unit tests (`july_unit_tests`) |
| `BUILD_BENCHMARKS` | ON (x64) | `keyboard_benchmark.exe` |
| `STRICT_LAYOUT` | OFF | Fail the build if any layout row is unconfirmed (`source=memory`) or any SutonnyMJ table row is unverified (`source=converter`). **Required for release builds.** |
| `BUILD_COMPANION` | ON (x64) | Tray/status companion `JulyBangla.exe` |
| `ENABLE_DIAGNOSTICS` | OFF | Privacy-safe diagnostic instrumentation |
| `JULY_SPECTRE` | OFF | Adds `/Qspectre`. It needs the VS component "MSVC Spectre-mitigated libs" (`Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre`) and is **required for release builds**. |

All targets compile with `/W4 /WX /permissive- /sdl /guard:cf` and use the static CRT
(`/MT`), so no VC++ redistributable is required.
