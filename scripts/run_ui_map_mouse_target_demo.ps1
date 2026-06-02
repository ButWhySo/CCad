param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$ProjectPath,
  [string]$Name = "ui-map-mouse-targets",
  [int]$InitialLoadMilliseconds = 5000,
  [int]$PerTargetMilliseconds = 800
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $Root $BuildDir
$Artifacts = Join-Path $Root "artifacts"
$ScreenshotDir = Join-Path $Artifacts "screenshots"
New-Item -ItemType Directory -Force -Path $ScreenshotDir | Out-Null

$env:PATH = "$QtBin;$env:PATH"

$Gui = Join-Path $BuildPath "ccad_gui.exe"
if (-not (Test-Path $Gui)) {
  throw "Missing GUI executable: $Gui"
}
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
  $ProjectPath = Join-Path $Root "artifacts\demos\sprint160-placement-crash-ci-final.ccad.json"
}
if (-not (Test-Path $ProjectPath)) {
  throw "Missing project path: $ProjectPath"
}

function Invoke-PreTestBeep {
  $beepPath = Join-Path $Root "docs\beep.mp3"
  if (-not (Test-Path $beepPath)) {
    return
  }
  Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CcadTargetWinMM {
  [DllImport("winmm.dll", CharSet = CharSet.Auto)]
  public static extern int mciSendString(string command, System.Text.StringBuilder buffer, int bufferSize, IntPtr hwndCallback);
}
"@
  [void][CcadTargetWinMM]::mciSendString("close ccad_target_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadTargetWinMM]::mciSendString("open `"$beepPath`" type mpegvideo alias ccad_target_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadTargetWinMM]::mciSendString("play ccad_target_beep", $null, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 350
  [void][CcadTargetWinMM]::mciSendString("close ccad_target_beep", $null, 0, [IntPtr]::Zero)
}

$stdoutLog = Join-Path $ScreenshotDir "$Name.stdout.log"
$stderrLog = Join-Path $ScreenshotDir "$Name.stderr.log"

Invoke-PreTestBeep
Start-Sleep -Seconds 2

$process = Start-Process -FilePath $Gui `
  -ArgumentList @("--test-ui-map-target-sequence", $ProjectPath, $ScreenshotDir, $Name,
                  [string]$InitialLoadMilliseconds, [string]$PerTargetMilliseconds) `
  -PassThru -Wait -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog

if ($process.ExitCode -ne 0) {
  throw "UI map target sequence failed with exit code $($process.ExitCode). Stdout: $stdoutLog Stderr: $stderrLog"
}

$Report = Join-Path $ScreenshotDir "$Name-target-sequence.json"
Write-Output "Report: $Report"
Write-Output "Stdout: $stdoutLog"
Write-Output "Stderr: $stderrLog"
