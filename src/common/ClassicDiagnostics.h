#pragma once

// Cold-path checks for Classic (SutonnyMJ) mode. Never called while typing.

namespace july {

struct ClassicDiagnostics {
    bool sutonnyInstalled = false;  // a font family named "SutonnyMJ" is installed
    unsigned ansiCodePage = 0;      // GetACP()
    bool ansiCodePageOk = false;    // 1252: Classic text round-trips through ANSI apps
};

// Reports whether Classic output will display and round-trip correctly. The product
// never downloads fonts; the UI shows these results as a diagnostic message instead.
[[nodiscard]] ClassicDiagnostics checkClassicEnvironment() noexcept;

} // namespace july
