param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$Name = "sprint-demo"
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $Root $BuildDir
$Artifacts = Join-Path $Root "artifacts"
$DemoDir = Join-Path $Artifacts "demos"
$ScreenshotDir = Join-Path $Artifacts "screenshots"
New-Item -ItemType Directory -Force -Path $DemoDir, $ScreenshotDir | Out-Null

$env:PATH = "$QtBin;$env:PATH"

$Ccad = Join-Path $BuildPath "ccad.exe"
$Gui = Join-Path $BuildPath "ccad_gui.exe"
if (-not (Test-Path $Ccad)) {
  throw "Missing CLI executable: $Ccad"
}
if (-not (Test-Path $Gui)) {
  throw "Missing GUI executable: $Gui"
}

function Invoke-Ccad {
  & $Ccad @args
  if ($LASTEXITCODE -ne 0) {
    throw "ccad command failed ($LASTEXITCODE): $args"
  }
}

$Project = Join-Path $DemoDir "$Name.ccad.json"
$Inspect = Join-Path $DemoDir "$Name.inspect.json"
$Validate = Join-Path $DemoDir "$Name.validate.json"
$Drc = Join-Path $DemoDir "$Name.drc.json"
$KiCadFootprint = Join-Path $DemoDir "$Name-R_0805_2012Metric.kicad_mod"
$ImportedFootprint = Join-Path $DemoDir "$Name-R_0805_2012Metric.ccad-footprint.json"
$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$Screenshot = Join-Path $ScreenshotDir "$Name-$Timestamp.png"

Invoke-Ccad init --name $Name --width-mm 42 --height-mm 28 --out $Project
Invoke-Ccad pcb add-pad --file $Project --id P1 --component U1 --pin 1 --net N1 --layer F.Cu --x-mm 5 --y-mm 6 --width-mm 1.5 --height-mm 1.0
Invoke-Ccad pcb add-via --file $Project --id V1 --net N1 --x-mm 8 --y-mm 9 --diameter-mm 0.8 --drill-mm 0.4
Invoke-Ccad pcb add-track --file $Project --id T1 --net N1 --layer F.Cu --start-x-mm 5 --start-y-mm 6 --end-x-mm 8 --end-y-mm 9 --width-mm 0.25
Invoke-Ccad inspect $Project | Set-Content -Encoding UTF8 $Inspect
Invoke-Ccad validate $Project | Set-Content -Encoding UTF8 $Validate
Invoke-Ccad drc $Project | Set-Content -Encoding UTF8 $Drc

@'
(footprint "R_0805_2012Metric"
  (version 20240101)
  (generator "ccad-demo")
  (pad "1" smd roundrect (at -0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
  (pad "2" smd roundrect (at 0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
)
'@ | Set-Content -Encoding UTF8 $KiCadFootprint
Invoke-Ccad lib import-footprint --in $KiCadFootprint --out $ImportedFootprint
Invoke-Ccad pcb place-footprint --file $Project --footprint $ImportedFootprint --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu

$Process = Start-Process -FilePath $Gui -ArgumentList $Project -PassThru
try {
  Start-Sleep -Seconds 2
  $Process.Refresh()
  if ($Process.MainWindowHandle -eq 0) {
    throw "GUI did not expose a main window handle"
  }

  Add-Type -AssemblyName System.Drawing
  if (-not ("CCad.NativeWindow" -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;

namespace CCad {
  public static class NativeWindow {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
      public int Left;
      public int Top;
      public int Right;
      public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
  }
}
"@
  }

  [CCad.NativeWindow]::SetForegroundWindow($Process.MainWindowHandle) | Out-Null
  Start-Sleep -Milliseconds 300
  $Rect = New-Object CCad.NativeWindow+RECT
  [CCad.NativeWindow]::GetWindowRect($Process.MainWindowHandle, [ref]$Rect) | Out-Null
  $Width = $Rect.Right - $Rect.Left
  $Height = $Rect.Bottom - $Rect.Top
  if ($Width -le 0 -or $Height -le 0) {
    throw "Invalid GUI window rectangle"
  }

  $Bitmap = New-Object System.Drawing.Bitmap $Width, $Height
  $Graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
  try {
    $Graphics.CopyFromScreen($Rect.Left, $Rect.Top, 0, 0, $Bitmap.Size)
    $Bitmap.Save($Screenshot, [System.Drawing.Imaging.ImageFormat]::Png)
  } finally {
    $Graphics.Dispose()
    $Bitmap.Dispose()
  }
} finally {
  if (-not $Process.HasExited) {
    $Process.CloseMainWindow() | Out-Null
    if (-not $Process.WaitForExit(2000)) {
      $Process.Kill()
    }
  }
}

Write-Output "Project: $Project"
Write-Output "Inspect: $Inspect"
Write-Output "Validate: $Validate"
Write-Output "DRC: $Drc"
Write-Output "KiCad footprint: $KiCadFootprint"
Write-Output "Imported footprint: $ImportedFootprint"
Write-Output "Screenshot: $Screenshot"
