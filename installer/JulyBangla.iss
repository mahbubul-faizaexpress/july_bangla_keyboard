; July Bangla Keyboard installer (Inno Setup 6).
;
; Build with tools\Build-Installer.ps1, which compiles the Release binaries and passes
; /DAppVersion and /DPreview. Installs per machine (TSF text services must be registered
; under HKLM and live where Store/AppContainer apps may load them), registers the 64-bit
; and 32-bit text-service DLLs with the matching regsvr32, and reverses everything on
; uninstall.

#ifndef AppVersion
  #error AppVersion must be defined (use tools\Build-Installer.ps1)
#endif
#ifdef Preview
  #define OutputSuffix "-preview"
#else
  #define OutputSuffix ""
#endif
; Where the built binaries are and where Setup is written (tools\ci overrides these).
#ifndef BuildRoot
  #define BuildRoot "..\build"
#endif
#ifndef OutDir
  #define OutDir "..\build\installer"
#endif

[Setup]
AppId={{47576C5C-9727-4729-94D4-B66A71DA7CB2}
AppName=July Bangla Keyboard
AppVersion={#AppVersion}
AppVerName=July Bangla Keyboard {#AppVersion}
AppPublisher=July Bangla Keyboard project
DefaultDirName={autopf}\JulyBanglaKeyboard
DefaultGroupName=July Bangla Keyboard
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir={#OutDir}
OutputBaseFilename=JulyBanglaKeyboard-{#AppVersion}{#OutputSuffix}-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=July Bangla Keyboard
SetupIconFile=..\resources\JulyBangla.ico
UninstallDisplayIcon={app}\JulyBangla.exe
VersionInfoVersion={#AppVersion}
VersionInfoProductName=July Bangla Keyboard
SetupLogging=yes
; Never close Explorer, browsers etc. to replace a loaded DLL: files in use are replaced
; at the next restart instead (restartreplace), and Setup asks for that restart.
CloseApplications=no
; The per-user settings below belong to the user who runs Setup (the normal case on a
; personal computer, where that user elevates with their own account).
UsedUserAreasWarning=no

[Tasks]
Name: "addkeyboard"; Description: "Add July Bangla Keyboard to my keyboard list (Win+Space)"
Name: "startup"; Description: "Show the status bar and tray icon when Windows starts"

[Files]
Source: "{#BuildRoot}\x64\src\tip\Release\JulyTip.dll"; DestDir: "{app}\x64"; Flags: ignoreversion restartreplace uninsrestartdelete regserver 64bit
Source: "{#BuildRoot}\x86\src\tip\Release\JulyTip.dll"; DestDir: "{app}\x86"; Flags: ignoreversion restartreplace uninsrestartdelete regserver 32bit
Source: "{#BuildRoot}\x64\src\app\Release\JulyBangla.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\tools\dev\Enable-Profile.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\SECURITY.md"; DestDir: "{app}"; DestName: "PRIVACY.md"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[InstallDelete]
; Leftovers of developer registrations (renamed loaded DLLs); skipped if still in use.
Type: files; Name: "{app}\x64\JulyTip.dll.old-*"
Type: files; Name: "{app}\x86\JulyTip.dll.old-*"

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "JulyBanglaKeyboard"; ValueData: """{app}\JulyBangla.exe"" --autostart"; Tasks: startup; Flags: uninsdeletevalue
; Also remove the startup value if the user enabled it later from the tray menu.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: none; ValueName: "JulyBanglaKeyboard"; Flags: uninsdeletevalue dontcreatekey
; Per-user settings (mode, status bar position); removed on uninstall.
Root: HKCU; Subkey: "Software\JulyBangla"; ValueType: none; Flags: uninsdeletekey dontcreatekey

[Icons]
Name: "{autoprograms}\July Bangla Keyboard"; Filename: "{app}\JulyBangla.exe"; Comment: "Bangla keyboard (Bijoy-compatible) status bar and tray icon"

[Run]
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\Enable-Profile.ps1"""; Flags: runhidden runasoriginaluser waituntilterminated; Tasks: addkeyboard; StatusMsg: "Adding the keyboard to your keyboard list..."
Filename: "{app}\JulyBangla.exe"; Description: "Start July Bangla Keyboard now"; Flags: nowait postinstall runasoriginaluser skipifsilent

[UninstallRun]
Filename: "taskkill.exe"; Parameters: "/IM JulyBangla.exe /F"; Flags: runhidden; RunOnceId: "StopCompanion"
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\Enable-Profile.ps1"" -Disable"; Flags: runhidden; RunOnceId: "RemoveKeyboard"

[Code]
// Stop a running companion so JulyBangla.exe can be replaced without a restart.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/IM JulyBangla.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;
