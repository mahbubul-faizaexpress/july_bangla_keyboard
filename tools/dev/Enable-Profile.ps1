# Adds (or removes) the July Bangla Keyboard input method in the current user's language
# list, using the documented Set-WinUserLanguageList cmdlet. No administrator rights.
#
#   powershell -ExecutionPolicy Bypass -File tools\dev\Enable-Profile.ps1 [-Disable]

param([switch]$Disable)

$ErrorActionPreference = 'Stop'
$tip = '0845:{AF6ABBB0-A4E4-4624-B2F2-0885AAAB8A7F}{CDFAB3B7-9E90-40DD-B04A-197223A26528}'

$list = Get-WinUserLanguageList
$bangla = $list | Where-Object { $_.LanguageTag -eq 'bn-BD' }

if ($Disable) {
    if ($bangla -and $bangla.InputMethodTips.Contains($tip)) {
        [void]$bangla.InputMethodTips.Remove($tip)
        Set-WinUserLanguageList $list -Force
        Write-Output 'July Bangla Keyboard removed from the language list.'
    } else {
        Write-Output 'July Bangla Keyboard was not in the language list.'
    }
    exit 0
}

if (-not $bangla) {
    $list.Add('bn-BD')
    $bangla = $list | Where-Object { $_.LanguageTag -eq 'bn-BD' }
}
if (-not $bangla.InputMethodTips.Contains($tip)) { $bangla.InputMethodTips.Add($tip) }
Set-WinUserLanguageList $list -Force

$check = (Get-WinUserLanguageList | Where-Object { $_.LanguageTag -eq 'bn-BD' }).InputMethodTips
if ($check -contains $tip) {
    Write-Output 'July Bangla Keyboard is in the language list (Win+Space to select it).'
    exit 0
}
Write-Output 'ERROR: Windows did not keep the input method in the list (is the text service registered?).'
exit 1
