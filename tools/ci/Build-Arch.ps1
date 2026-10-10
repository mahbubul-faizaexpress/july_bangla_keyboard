<#
.SYNOPSIS
    Release build and tests for one architecture with whichever Visual Studio is installed.

.DESCRIPTION
    Used by the CI and release workflows. (tools\Build-Installer.ps1 is the developer
    script and uses the CMake presets, which name one Visual Studio version exactly.)
    Output layout matches the presets: <BuildRoot>\<Arch>\src\...\Release\.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ci\Build-Arch.ps1 -Arch x64
#>
param(
    [Parameter(Mandatory)][ValidateSet('x64', 'x86')][string]$Arch,
    [string]$BuildRoot = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'build')
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'No Visual Studio with the C++ tools was found.' }
Write-Host "Visual Studio: $vs"
$devArch = if ($Arch -eq 'x64') { 'amd64' } else { 'x86' }
& (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch $devArch -HostArch amd64 -SkipAutomaticLocation | Out-Null

$build = Join-Path $BuildRoot $Arch
cmake -S $repo -B $build -G 'Ninja Multi-Config' -DSTRICT_LAYOUT=ON -DBUILD_BENCHMARKS=OFF
if ($LASTEXITCODE -ne 0) { throw "configure $Arch failed" }
cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "build $Arch failed" }
ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests $Arch failed" }
