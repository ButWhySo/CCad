param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$Name = "sprint-demo",
  [switch]$ClickSelection,
  [int]$GuiWaitSeconds = 20,
  [switch]$PreferInternalScreenshot
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $Root $BuildDir
$Artifacts = Join-Path $Root "artifacts"
$DemoDir = Join-Path $Artifacts "demos"
$ScreenshotDir = Join-Path $Artifacts "screenshots"
New-Item -ItemType Directory -Force -Path $DemoDir, $ScreenshotDir | Out-Null

$env:PATH = "$QtBin;$env:PATH"

$preflight = Join-Path $PSScriptRoot "preflight_qt_env.ps1"
if (Test-Path $preflight) {
  & $preflight -BuildDir $BuildDir | Out-Null
}

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

function Invoke-CcadDrcReport {
  $Output = & $Ccad drc $Project
  $ExitCode = $LASTEXITCODE
  $Output | Set-Content -Encoding UTF8 $Drc
  if ($ExitCode -eq 2) {
    throw "ccad drc command failed with usage/file/parse error ($ExitCode): $Project"
  }
}

function Save-GuiWindowScreenshot {
  param(
    [Parameter(Mandatory = $true)]
    [string]$GuiPath,
    [Parameter(Mandatory = $true)]
    [string]$ProjectPath,
    [Parameter(Mandatory = $true)]
    [string]$ScreenshotPath,
    [Parameter(Mandatory = $true)]
    [int]$WaitSeconds
  )

  Add-Type -AssemblyName System.Drawing
  Add-Type -AssemblyName System.Windows.Forms
  Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class NativeWin {
  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")]
  public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [DllImport("user32.dll")]
  public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
}
"@

  $stdoutLog = Join-Path $ScreenshotDir ("gui-capture-" + [Guid]::NewGuid().ToString("N") + ".stdout.log")
  $stderrLog = Join-Path $ScreenshotDir ("gui-capture-" + [Guid]::NewGuid().ToString("N") + ".stderr.log")
  $process = Start-Process -FilePath $GuiPath -ArgumentList $ProjectPath -PassThru `
    -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
  try {
    $deadline = (Get-Date).AddSeconds([Math]::Max(5, $WaitSeconds))
    $windowReady = $false
    while ((Get-Date) -lt $deadline) {
      $process.Refresh()
      if ($process.HasExited) {
        throw "ccad_gui exited before screenshot fallback capture."
      }
      if ($process.MainWindowHandle -ne 0) {
        $windowReady = $true
        break
      }
      Start-Sleep -Milliseconds 250
    }
    if (-not $windowReady -or $process.MainWindowHandle -eq 0) {
      throw "ccad_gui window handle not available in fallback capture."
    }

    # Even after a window handle appears, allow full UI/layout/project render to settle.
    Start-Sleep -Milliseconds ([Math]::Max(15000, $WaitSeconds * 1000))

    $process.Refresh()
    if ($process.HasExited) {
      $process.WaitForExit()
      $exitCodeText = "unknown"
      try {
        $exitCodeText = [string]$process.ExitCode
      } catch {
        $exitCodeText = "unavailable"
      }
      $stdoutText = ""
      $stderrText = ""
      if (Test-Path $stdoutLog) { $stdoutText = Get-Content -Raw $stdoutLog }
      if (Test-Path $stderrLog) { $stderrText = Get-Content -Raw $stderrLog }
      throw "ccad_gui exited before screenshot capture. exit=$exitCodeText`nstdout:`n$stdoutText`nstderr:`n$stderrText"
    }
    if ($process.MainWindowHandle -eq 0) {
      throw "ccad_gui main window handle disappeared before capture."
    }
    if ([string]::IsNullOrWhiteSpace($process.MainWindowTitle)) {
      throw "ccad_gui main window title was empty before capture."
    }

    [NativeWin]::ShowWindow($process.MainWindowHandle, 9) | Out-Null
    [NativeWin]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 1200

    $rect = New-Object NativeWin+RECT
    if (-not [NativeWin]::GetWindowRect($process.MainWindowHandle, [ref]$rect)) {
      throw "GetWindowRect failed for fallback capture."
    }
    $width = [Math]::Max(1, $rect.Right - $rect.Left)
    $height = [Math]::Max(1, $rect.Bottom - $rect.Top)
    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $bitmap.Save($ScreenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $graphics.Dispose()
    $bitmap.Dispose()
  } finally {
    if ($process -and -not $process.HasExited) {
      [void]$process.CloseMainWindow()
      if (-not $process.WaitForExit(5000)) {
        Stop-Process -Id $process.Id -Force
      }
    }
  }
}

function Invoke-PreScreenshotBeep {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RootPath
  )

  $beepPath = Join-Path $RootPath "docs\beep.mp3"
  if (-not (Test-Path $beepPath)) {
    return
  }

  try {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class WinMMBridge {
  [DllImport("winmm.dll", CharSet = CharSet.Auto)]
  public static extern int mciSendString(string command, System.Text.StringBuilder buffer, int bufferSize, IntPtr hwndCallback);
}
"@
    [void][WinMMBridge]::mciSendString("close ccad_beep", $null, 0, [IntPtr]::Zero)
    $quoted = '"' + $beepPath + '"'
    [void][WinMMBridge]::mciSendString("open $quoted type mpegvideo alias ccad_beep", $null, 0, [IntPtr]::Zero)
    [void][WinMMBridge]::mciSendString("play ccad_beep", $null, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 350
    [void][WinMMBridge]::mciSendString("close ccad_beep", $null, 0, [IntPtr]::Zero)
  } catch {
    # Audio feedback is best-effort only.
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
Invoke-Ccad pcb add-track --file $Project --id T2 --net N2 --layer F.Cu --start-x-mm 5 --start-y-mm 9 --end-x-mm 8 --end-y-mm 6 --width-mm 0.25
Invoke-Ccad pcb add-placement-region --file $Project --id PR1 --kind component --x-mm 11 --y-mm 4 --width-mm 12 --height-mm 8
Invoke-Ccad pcb add-keepout --file $Project --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3

$ProjectObject = Get-Content -Raw $Project | ConvertFrom-Json
$ProjectObject.nets = @(
  [ordered]@{ id = "N1"; members = @() },
  [ordered]@{ id = "N2"; members = @() }
)
$ProjectJson = $ProjectObject | ConvertTo-Json -Depth 32
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Project, $ProjectJson, $Utf8NoBom)

Invoke-Ccad inspect $Project | Set-Content -Encoding UTF8 $Inspect
Invoke-Ccad validate $Project | Set-Content -Encoding UTF8 $Validate
Invoke-CcadDrcReport

@'
(footprint "R_0805_2012Metric"
  (version 20240101)
  (generator "ccad-demo")
  (pad "1" smd roundrect (at -0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
  (pad "2" smd roundrect (at 0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
)
'@ | Set-Content -Encoding UTF8 $KiCadFootprint
Invoke-Ccad lib import-footprint --in $KiCadFootprint --out $ImportedFootprint
Invoke-Ccad pcb place-footprint --file $Project --footprint $ImportedFootprint --component R1 --at-x-mm 16 --at-y-mm 14 --layer F.Cu --rotation-deg 90

Invoke-PreScreenshotBeep -RootPath $Root
Start-Sleep -Seconds 2

if ($PreferInternalScreenshot) {
  $GuiOutput = & $Gui --screenshot $Project $Screenshot
  $GuiExitCode = $LASTEXITCODE
  if ($GuiExitCode -ne 0 -or -not (Test-Path $Screenshot)) {
    $internalOutput = ($GuiOutput | Out-String)
    if ($GuiExitCode -ne 0) {
      Write-Warning "Internal screenshot mode failed. exit=$GuiExitCode output=$internalOutput"
    }
    Save-GuiWindowScreenshot -GuiPath $Gui -ProjectPath $Project -ScreenshotPath $Screenshot -WaitSeconds $GuiWaitSeconds
    if (-not (Test-Path $Screenshot)) {
      throw "GUI screenshot fallback did not create: $Screenshot"
    }
  }
} else {
  Save-GuiWindowScreenshot -GuiPath $Gui -ProjectPath $Project -ScreenshotPath $Screenshot -WaitSeconds $GuiWaitSeconds
  if (-not (Test-Path $Screenshot)) {
    throw "GUI screenshot capture did not create: $Screenshot"
  }
}

Write-Output "Project: $Project"
Write-Output "Inspect: $Inspect"
Write-Output "Validate: $Validate"
Write-Output "DRC: $Drc"
Write-Output "KiCad footprint: $KiCadFootprint"
Write-Output "Imported footprint: $ImportedFootprint"
Write-Output "Screenshot: $Screenshot"
