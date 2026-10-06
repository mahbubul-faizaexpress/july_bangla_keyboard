# Developer registration of the July Bangla Keyboard text service (until the MSI exists).
#
# Must run elevated. Copies the Release TIP DLLs into Program Files (a location that
# AppContainer/Store apps are allowed to load from), then registers both bitnesses with
# the matching regsvr32. Writes a log to %TEMP%\JulyBangla-Register.log.
#
#   powershell -ExecutionPolicy Bypass -File tools\dev\Register-Dev.ps1 [-Unregister]

param([switch]$Unregister)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$installDir = Join-Path $env:ProgramFiles 'JulyBanglaKeyboard'
$log = Join-Path $env:TEMP 'JulyBangla-Register.log'
$targets = @(
    @{ Source = "$repo\build\x64\src\tip\Release\JulyTip.dll"; Dest = "$installDir\x64\JulyTip.dll"; Regsvr = "$env:SystemRoot\System32\regsvr32.exe" },
    @{ Source = "$repo\build\x86\src\tip\Release\JulyTip.dll"; Dest = "$installDir\x86\JulyTip.dll"; Regsvr = "$env:SystemRoot\SysWOW64\regsvr32.exe" }
)

function Write-Log([string]$message) {
    $line = "{0:u} {1}" -f (Get-Date), $message
    Add-Content -Path $log -Value $line -Encoding UTF8
    Write-Output $line
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Log 'ERROR: must run as administrator'
    exit 5
}

$failed = $false
foreach ($t in $targets) {
    if ($Unregister) {
        if (Test-Path $t.Dest) {
            $p = Start-Process -FilePath $t.Regsvr -ArgumentList '/s', '/u', "`"$($t.Dest)`"" -Wait -PassThru
            Write-Log ("unregister {0}: exit {1}" -f $t.Dest, $p.ExitCode)
            if ($p.ExitCode -ne 0) { $failed = $true }
        }
        continue
    }

    if (-not (Test-Path $t.Source)) {
        Write-Log "ERROR: missing build output $($t.Source)"
        $failed = $true
        continue
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $t.Dest) | Out-Null
    if (Test-Path $t.Dest) {
        # A loaded DLL cannot be overwritten, but it can be renamed; running apps keep the
        # old copy until they restart.
        $stale = "$($t.Dest).old-{0:yyyyMMddHHmmss}" -f (Get-Date)
        Rename-Item -Path $t.Dest -NewName (Split-Path $stale -Leaf)
        Write-Log "renamed previous DLL to $stale"
    }
    Copy-Item -Path $t.Source -Destination $t.Dest
    $p = Start-Process -FilePath $t.Regsvr -ArgumentList '/s', "`"$($t.Dest)`"" -Wait -PassThru
    Write-Log ("register {0}: exit {1}" -f $t.Dest, $p.ExitCode)
    if ($p.ExitCode -ne 0) { $failed = $true }
}

$companionSource = "$repo\build\x64\src\app\Release\JulyBangla.exe"
$companionDest = "$installDir\JulyBangla.exe"
if ($Unregister) {
    Get-Process JulyBangla -ErrorAction SilentlyContinue | Stop-Process -Force
    Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'JulyBanglaKeyboard' -ErrorAction SilentlyContinue
    Write-Log 'stopped the companion and removed its startup entry'
} elseif (Test-Path $companionSource) {
    Get-Process JulyBangla -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Milliseconds 300
    Copy-Item -Path $companionSource -Destination $companionDest -Force
    Write-Log "installed companion $companionDest"
}

if (-not $Unregister) {
    # Running programs keep the DLL version they loaded until they restart.
    $holders = Get-Process | Where-Object {
        try { $_.Modules | Where-Object { $_.ModuleName -like 'JulyTip*' } } catch { $null }
    } | Sort-Object ProcessName -Unique
    foreach ($p in $holders) {
        Write-Log ("RESTART NEEDED: {0} (pid {1}) still runs the previous version" -f $p.ProcessName, $p.Id)
    }
}

if ($Unregister -and -not $failed -and (Test-Path $installDir)) {
    Remove-Item -Recurse -Force -Path $installDir -ErrorAction SilentlyContinue
    Write-Log "removed $installDir (files still loaded by running apps may remain until reboot)"
}
if ($failed) { exit 1 } else { exit 0 }
