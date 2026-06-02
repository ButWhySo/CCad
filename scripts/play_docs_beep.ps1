param(
  [string]$RootPath = "",
  [int]$PostDelaySeconds = 2
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RootPath)) {
  $RootPath = Resolve-Path (Join-Path $PSScriptRoot "..")
}

$beepPath = Join-Path $RootPath "docs\beep.mp3"
if (Test-Path $beepPath) {
  try {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CcadDocsBeep {
  [DllImport("winmm.dll", CharSet = CharSet.Auto)]
  public static extern int mciSendString(string command, System.Text.StringBuilder buffer, int bufferSize, IntPtr hwndCallback);
}
"@
    [void][CcadDocsBeep]::mciSendString("close ccad_docs_beep", $null, 0, [IntPtr]::Zero)
    $quoted = '"' + $beepPath + '"'
    [void][CcadDocsBeep]::mciSendString("open $quoted type mpegvideo alias ccad_docs_beep", $null, 0, [IntPtr]::Zero)
    [void][CcadDocsBeep]::mciSendString("play ccad_docs_beep", $null, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 350
    [void][CcadDocsBeep]::mciSendString("close ccad_docs_beep", $null, 0, [IntPtr]::Zero)
  } catch {
    Write-Warning "Docs beep failed: $($_.Exception.Message)"
  }
}

if ($PostDelaySeconds -gt 0) {
  Start-Sleep -Seconds $PostDelaySeconds
}
