@echo off
rem Double-click this file to run the July Bangla Keyboard end-to-end test.
rem Windows only lets a program you started yourself come to the front, which the test needs.
chcp 65001 >nul
cd /d "%~dp0"
echo.
echo  July Bangla Keyboard - keyboard test. Please wait a few seconds...
echo.
"build\x64\tests\Release\july_tip_smoke.exe" > "build\keyboard-test-result.txt" 2>&1
type "build\keyboard-test-result.txt"
echo.
echo  Done. You can close this window.
pause >nul
