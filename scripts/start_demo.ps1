$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path ".\").Path
$Gui = "$Root\build-qt\ccad_gui.exe"
$Project = "$Root\cli_demo.json"

Write-Output "Starting GUI Maximized..."
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
$GuiProcess = Start-Process -FilePath $Gui -ArgumentList $Project -PassThru -WindowStyle Maximized

Start-Sleep -Seconds 2

& powershell -ExecutionPolicy Bypass -File "$Root\scripts\live_demo_steps.ps1"

Write-Output "Taking screenshot..."
$ScreenshotFile = "$Root\artifacts\screenshots\live_demo_final.png"
if (-not (Test-Path "$Root\artifacts\screenshots")) {
    New-Item -ItemType Directory -Path "$Root\artifacts\screenshots"
}
& $Gui --screenshot $Project $ScreenshotFile

Write-Output "Done! You can close the GUI window now."
