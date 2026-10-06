# Builds the Release binaries (x64 + x86) and the Inno Setup installer.
#
#   powershell -ExecutionPolicy Bypass -File tools\Build-Installer.ps1 [-Release]
#
# Without -Release the installer is a "-preview": the SutonnyMJ Classic table is not yet
# verified in the font, so a strict (release) build would refuse it. -Release builds with
# STRICT_LAYOUT=ON and fails until every layout and Classic row is verified.

param([switch]$Release)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$cmakeBin = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$cmake = Join-Path $cmakeBin 'cmake.exe'
$ctest = Join-Path $cmakeBin 'ctest.exe'
$iscc = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe") |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not (Test-Path $cmake)) { throw "CMake not found at $cmake" }
if (-not $iscc) { throw 'Inno Setup 6 (ISCC.exe) not found: winget install JRSoftware.InnoSetup' }

$version = (Select-String -Path "$repo\CMakeLists.txt" -Pattern '^\s*VERSION\s+(\d+\.\d+\.\d+)').Matches[0].Groups[1].Value
$strict = if ($Release) { 'ON' } else { 'OFF' }

Push-Location $repo
try {
    foreach ($preset in 'x64', 'x86') {
        & $cmake --preset $preset "-DSTRICT_LAYOUT=$strict" | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "configure $preset failed" }
        & $cmake --build --preset "$preset-release"
        if ($LASTEXITCODE -ne 0) { throw "build $preset failed" }
        & $ctest --preset "$preset-release"
        if ($LASTEXITCODE -ne 0) { throw "tests $preset failed" }
    }
    $defines = @("/DAppVersion=$version")
    if (-not $Release) { $defines += '/DPreview' }
    & $iscc @defines "$repo\installer\JulyBangla.iss"
    if ($LASTEXITCODE -ne 0) { throw 'ISCC failed' }
} finally {
    Pop-Location
}
Get-ChildItem "$repo\build\installer\*.exe" | Sort-Object LastWriteTime | Select-Object -Last 1 |
    ForEach-Object { 'Installer: {0} ({1:N0} KB)' -f $_.FullName, ($_.Length / 1KB) }
