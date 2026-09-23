param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$ProjectPath,
  [string]$Name = "ui-map-mouse-targets",
  [int]$InitialLoadMilliseconds = 5000,
  [int]$PerTargetMilliseconds = 800,
  [switch]$RequireNativeToolCatalog
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

$priorThreshold = $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS
$priorTraceDebug = $env:CCAD_TRACE_DEBUG
if ($Name.StartsWith("sprint969-context")) {
  # Exercise the real large-context branch with a deliberately low, valid
  # threshold. The /context feature remains local and never invokes the model.
  $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS = "512"
  $env:CCAD_TRACE_DEBUG = "1"
}
try {
  $process = Start-Process -FilePath $Gui `
    -ArgumentList @("--test-ui-map-target-sequence", $ProjectPath, $ScreenshotDir, $Name,
                    [string]$InitialLoadMilliseconds, [string]$PerTargetMilliseconds) `
    -PassThru -Wait -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
} finally {
  if ($null -eq $priorThreshold) { Remove-Item Env:CCAD_AGENT_LARGE_CONTEXT_TOKENS -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS = $priorThreshold }
  if ($null -eq $priorTraceDebug) { Remove-Item Env:CCAD_TRACE_DEBUG -ErrorAction SilentlyContinue }
  else { $env:CCAD_TRACE_DEBUG = $priorTraceDebug }
}

if ($process.ExitCode -ne 0) {
  throw "UI map target sequence failed with exit code $($process.ExitCode). Stdout: $stdoutLog Stderr: $stderrLog"
}

$Report = Join-Path $ScreenshotDir "$Name-target-sequence.json"
if ($RequireNativeToolCatalog) {
  $reportData = Get-Content -Raw $Report | ConvertFrom-Json
  if (-not $reportData.catalog_startup_verified) {
    throw "Native agent tool catalog was not installed before persisted provider activation. Report: $Report"
  }
}
Write-Output "Report: $Report"
Write-Output "Stdout: $stdoutLog"
Write-Output "Stderr: $stderrLog"
