; Inno Setup script for MiKoBots Studio (ARM version).
; Build the app first (packaging\mikobots_studio.spec), then compile this script;
; packaging\build.ps1 does both and passes BuildDir / OutDir (the release folder outside the repo).

#define AppName "MiKoBots Studio ARM"
#define AppVersion "1.15-arm"
#define AppExe "MiKoBots Studio.exe"
#ifndef BuildDir
  #define BuildDir "..\..\..\release\dist\MiKoBots Studio"
#endif
#ifndef OutDir
  #define OutDir "..\..\..\release\Output"
#endif

[Setup]
AppId={{6F3C2A51-8D4E-4B7A-9C1F-2E5D8A0B7C34}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Robotic arm controller
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile=..\MiKoBots_Studio\assets\images\mikobot.ico
OutputDir={#OutDir}
OutputBaseFilename=MiKoBots Studio ARM setup
Compression=lzma2/fast
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
; the app only writes to Documents\MyAppData, so no admin rights are needed
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
