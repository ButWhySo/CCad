param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$ProjectPath,
  [string]$Name = "gui-interaction",
  [ValidateSet("Footprint", "Symbol")]
  [string]$Mode = "Footprint",
  [int]$GuiWaitSeconds = 7
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
  $ProjectPath = Join-Path $Root "artifacts\demos\sprint156-lazy-library-chooser-preview-final.ccad.json"
}
if (-not (Test-Path $ProjectPath)) {
  throw "Missing project path: $ProjectPath"
}

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CcadInput {
  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")]
  public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [DllImport("user32.dll")]
  public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
  [DllImport("user32.dll")]
  public static extern bool SetCursorPos(int X, int Y);
  [DllImport("user32.dll")]
  public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
  [DllImport("user32.dll")]
  public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
}
"@

function Invoke-Click {
  param([int]$X, [int]$Y)
  [CcadInput]::SetCursorPos($X, $Y) | Out-Null
  Start-Sleep -Milliseconds 120
  [CcadInput]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 80
  [CcadInput]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Invoke-Key {
  param([byte]$VirtualKey)
  [CcadInput]::keybd_event($VirtualKey, 0, 0, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds 80
  [CcadInput]::keybd_event($VirtualKey, 0, 0x0002, [UIntPtr]::Zero)
}

function Invoke-PreTestBeep {
  $beepPath = Join-Path $Root "docs\beep.mp3"
  if (-not (Test-Path $beepPath)) {
    return
  }
  Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CcadWinMM {
  [DllImport("winmm.dll", CharSet = CharSet.Auto)]
  public static extern int mciSendString(string command, System.Text.StringBuilder buffer, int bufferSize, IntPtr hwndCallback);
}
"@
  [void][CcadWinMM]::mciSendString("close ccad_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadWinMM]::mciSendString("open `"$beepPath`" type mpegvideo alias ccad_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadWinMM]::mciSendString("play ccad_beep", $null, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 350
  [void][CcadWinMM]::mciSendString("close ccad_beep", $null, 0, [IntPtr]::Zero)
}

function Save-WindowScreenshot {
  param([IntPtr]$Handle, [string]$Path)
  $rect = New-Object CcadInput+RECT
  if (-not [CcadInput]::GetWindowRect($Handle, [ref]$rect)) {
    throw "GetWindowRect failed."
  }
  $width = [Math]::Max(1, $rect.Right - $rect.Left)
  $height = [Math]::Max(1, $rect.Bottom - $rect.Top)
  $bitmap = New-Object System.Drawing.Bitmap($width, $height)
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
  $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
  $graphics.Dispose()
  $bitmap.Dispose()
}

$stdoutLog = Join-Path $ScreenshotDir "$Name.stdout.log"
$stderrLog = Join-Path $ScreenshotDir "$Name.stderr.log"
$process = Start-Process -FilePath $Gui -ArgumentList $ProjectPath -PassThru `
  -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog

try {
  $deadline = (Get-Date).AddSeconds(20)
  while ((Get-Date) -lt $deadline) {
    $process.Refresh()
    if ($process.HasExited) {
      throw "ccad_gui exited before interaction."
    }
    if ($process.MainWindowHandle -ne 0) {
      break
    }
    Start-Sleep -Milliseconds 250
  }
  if ($process.MainWindowHandle -eq 0) {
    throw "ccad_gui did not expose a main window handle."
  }

  Invoke-PreTestBeep
  Start-Sleep -Seconds 2

  [CcadInput]::ShowWindow($process.MainWindowHandle, 9) | Out-Null
  [CcadInput]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
  Start-Sleep -Milliseconds 1000

  $rect = New-Object CcadInput+RECT
  [CcadInput]::GetWindowRect($process.MainWindowHandle, [ref]$rect) | Out-Null
  if ($Mode -eq "Symbol") {
    Invoke-Click -X ($rect.Left + 535) -Y ($rect.Top + 120)
    Start-Sleep -Milliseconds 500
  }

  Invoke-Click -X ($rect.Left + 650) -Y ($rect.Top + 350)
  Start-Sleep -Milliseconds 400
  if ($Mode -eq "Symbol") {
    Invoke-Key -VirtualKey 0x41
  } else {
    Invoke-Key -VirtualKey 0x4F
  }
  Start-Sleep -Seconds 3

  $process.Refresh()
  [CcadInput]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
  $chooserRowX = $rect.Left + 315
  $chooserRowY = $rect.Top + 165
  Invoke-Click -X $chooserRowX -Y $chooserRowY

  Start-Sleep -Seconds ([Math]::Max(1, $GuiWaitSeconds))
  $Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $Screenshot = Join-Path $ScreenshotDir "$Name-$Timestamp.png"
  Save-WindowScreenshot -Handle $process.MainWindowHandle -Path $Screenshot
  Write-Output "Screenshot: $Screenshot"
  Write-Output "Stdout: $stdoutLog"
  Write-Output "Stderr: $stderrLog"
} finally {
  if ($process -and -not $process.HasExited) {
    [void]$process.CloseMainWindow()
    if (-not $process.WaitForExit(5000)) {
      Stop-Process -Id $process.Id -Force
    }
  }
}
