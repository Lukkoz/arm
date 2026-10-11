# Builds MiKoBots Studio (ARM version): the app folder with the exe, then the Windows installer.
# Run from anywhere:  powershell -ExecutionPolicy Bypass -File packaging\build.ps1 [-Release <folder>]
# Needs: the app venv with PyInstaller (pip install pyinstaller), and Inno Setup 6.
#
# The output goes outside the repository (default: the folder that contains the repo, \release),
# because this folder sits inside the Arduino sketch and arduino-cli scans everything in it.
param([string]$Release = "", [string]$Python = "")
$ErrorActionPreference = "Stop"
$app = Split-Path $PSScriptRoot -Parent                     # ...\arm\MiKoBots-Studio
$outside = Split-Path (Split-Path $app -Parent) -Parent     # the folder that contains the repo
if (-not $Release) { $Release = Join-Path $outside "release" }
# the venv lives outside the repo too (default: <outside>\.venv), see README
if (-not $Python) {
    $Python = @("$outside\.venv\Scripts\python.exe", "$app\.venv\Scripts\python.exe") |
              Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Python) { throw "No Python venv found; pass -Python <path to python.exe>" }
New-Item -ItemType Directory -Force $Release | Out-Null

& $Python -m PyInstaller "$PSScriptRoot\mikobots_studio.spec" --noconfirm `
    --distpath "$Release\dist" --workpath "$Release\build"
if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed" }

$iscc = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
          "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
          "$env:ProgramFiles\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw "Inno Setup 6 not found (winget install JRSoftware.InnoSetup)" }
& $iscc "/DBuildDir=$Release\dist\MiKoBots Studio" "/DOutDir=$Release\Output" "$PSScriptRoot\installer.iss"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

Write-Host "App:       $Release\dist\MiKoBots Studio\MiKoBots Studio.exe"
Write-Host "Installer: $Release\Output\MiKoBots Studio ARM setup.exe"
