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
$RouteJob = Join-Path $DemoDir "$Name.route-job.json"
$RouteStatus = Join-Path $DemoDir "$Name.route-status.json"
$KiCadFootprint = Join-Path $DemoDir "$Name-R_0805_2012Metric.kicad_mod"
$ImportedFootprint = Join-Path $DemoDir "$Name-R_0805_2012Metric.ccad-footprint.json"
$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$Screenshot = Join-Path $ScreenshotDir "$Name-$Timestamp.png"

Invoke-Ccad init --name $Name --width-mm 42 --height-mm 28 --out $Project
Invoke-Ccad pcb set-outline --file $Project --x-mm 2 --y-mm 2 --width-mm 44 --height-mm 30
Invoke-Ccad pcb set-rules --file $Project --copper-clearance-mm 0.20 --min-track-width-mm 0.15 --min-via-annular-ring-mm 0.10
Invoke-Ccad pcb add-standard-layers --file $Project
Invoke-Ccad pcb set-layer-visibility --file $Project --id In1.Cu --visible true
Invoke-Ccad pcb set-layer-visibility --file $Project --id In1.Cu --visible false
Invoke-Ccad pcb add-pad --file $Project --id JAC1.1 --component JAC1 --pin 1 --net AC1 --layer F.Cu --x-mm 8 --y-mm 17 --width-mm 1.8 --height-mm 1.4
Invoke-Ccad pcb add-pad --file $Project --id JAC2.1 --component JAC2 --pin 1 --net AC2 --layer F.Cu --x-mm 32 --y-mm 17 --width-mm 1.8 --height-mm 1.4
Invoke-Ccad pcb add-pad --file $Project --id JDC1.1 --component JDC1 --pin 1 --net DC_POS --layer F.Cu --x-mm 20 --y-mm 8 --width-mm 1.8 --height-mm 1.4
Invoke-Ccad pcb add-pad --file $Project --id JDC2.1 --component JDC2 --pin 1 --net DC_NEG --layer F.Cu --x-mm 20 --y-mm 26 --width-mm 1.8 --height-mm 1.4
Invoke-Ccad pcb add-pad --file $Project --id D1.A --component D1 --pin A --net AC1 --layer F.Cu --x-mm 10 --y-mm 15 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D1.K --component D1 --pin K --net DC_POS --layer F.Cu --x-mm 18 --y-mm 10 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D2.A --component D2 --pin A --net AC2 --layer F.Cu --x-mm 30 --y-mm 15 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D2.K --component D2 --pin K --net DC_POS --layer F.Cu --x-mm 22 --y-mm 10 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D3.A --component D3 --pin A --net DC_NEG --layer F.Cu --x-mm 18 --y-mm 24 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D3.K --component D3 --pin K --net AC1 --layer F.Cu --x-mm 10 --y-mm 19 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D4.A --component D4 --pin A --net DC_NEG --layer F.Cu --x-mm 22 --y-mm 24 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id D4.K --component D4 --pin K --net AC2 --layer F.Cu --x-mm 30 --y-mm 19 --width-mm 1.3 --height-mm 1.0
Invoke-Ccad pcb add-pad --file $Project --id C1.1 --component C1 --pin 1 --net DC_POS --layer F.Cu --x-mm 25 --y-mm 10 --width-mm 1.4 --height-mm 1.2
Invoke-Ccad pcb add-pad --file $Project --id C1.2 --component C1 --pin 2 --net DC_NEG --layer F.Cu --x-mm 25 --y-mm 24 --width-mm 1.4 --height-mm 1.2
Invoke-Ccad pcb add-pad --file $Project --id RLOAD.1 --component RLOAD --pin 1 --net DC_POS --layer F.Cu --x-mm 36 --y-mm 12 --width-mm 1.4 --height-mm 1.2
Invoke-Ccad pcb add-pad --file $Project --id RLOAD.2 --component RLOAD --pin 2 --net DC_NEG --layer F.Cu --x-mm 36 --y-mm 22 --width-mm 1.4 --height-mm 1.2
Invoke-Ccad pcb add-via --file $Project --id VPOS --net DC_POS --x-mm 28 --y-mm 10 --diameter-mm 0.8 --drill-mm 0.4
Invoke-Ccad pcb add-via --file $Project --id VNEG --net DC_NEG --x-mm 28 --y-mm 24 --diameter-mm 0.8 --drill-mm 0.4
Invoke-Ccad pcb add-track --file $Project --id TAC1.1 --net AC1 --layer F.Cu --start-x-mm 8 --start-y-mm 17 --end-x-mm 10 --end-y-mm 15 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TAC1.2 --net AC1 --layer F.Cu --start-x-mm 8 --start-y-mm 17 --end-x-mm 10 --end-y-mm 19 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TAC2.1 --net AC2 --layer F.Cu --start-x-mm 32 --start-y-mm 17 --end-x-mm 30 --end-y-mm 15 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TAC2.2 --net AC2 --layer F.Cu --start-x-mm 32 --start-y-mm 17 --end-x-mm 30 --end-y-mm 19 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TPOS.1 --net DC_POS --layer F.Cu --start-x-mm 18 --start-y-mm 10 --end-x-mm 20 --end-y-mm 8 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TPOS.2 --net DC_POS --layer F.Cu --start-x-mm 22 --start-y-mm 10 --end-x-mm 20 --end-y-mm 8 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TNEG.1 --net DC_NEG --layer F.Cu --start-x-mm 18 --start-y-mm 24 --end-x-mm 20 --end-y-mm 26 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TNEG.2 --net DC_NEG --layer F.Cu --start-x-mm 22 --start-y-mm 24 --end-x-mm 20 --end-y-mm 26 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TLOAD.1 --net DC_POS --layer F.Cu --start-x-mm 28 --start-y-mm 10 --end-x-mm 36 --end-y-mm 12 --width-mm 0.25
Invoke-Ccad pcb add-track --file $Project --id TLOAD.2 --net DC_NEG --layer F.Cu --start-x-mm 28 --start-y-mm 24 --end-x-mm 36 --end-y-mm 22 --width-mm 0.25

$ProjectObject = Get-Content -Raw $Project | ConvertFrom-Json
$ProjectObject.components = @(
  [ordered]@{ id = "JAC1"; part = "AC input"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
  [ordered]@{ id = "JAC2"; part = "AC input"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
  [ordered]@{ id = "JDC1"; part = "DC positive"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
  [ordered]@{ id = "JDC2"; part = "DC negative"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
  [ordered]@{ id = "D1"; part = "Diode bridge leg"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
  [ordered]@{ id = "D2"; part = "Diode bridge leg"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
  [ordered]@{ id = "D3"; part = "Diode bridge leg"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
  [ordered]@{ id = "D4"; part = "Diode bridge leg"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
  [ordered]@{ id = "C1"; part = "Bulk capacitor"; pins = @([ordered]@{ name = "1"; kind = "passive" }, [ordered]@{ name = "2"; kind = "passive" }) },
  [ordered]@{ id = "RLOAD"; part = "Load resistor"; pins = @([ordered]@{ name = "1"; kind = "passive" }, [ordered]@{ name = "2"; kind = "passive" }) }
)
$ProjectObject.nets = @(
  [ordered]@{ id = "AC1"; members = @(
    [ordered]@{ component_id = "JAC1"; pin_name = "1" },
    [ordered]@{ component_id = "D1"; pin_name = "A" },
    [ordered]@{ component_id = "D3"; pin_name = "K" }
  ) },
  [ordered]@{ id = "AC2"; members = @(
    [ordered]@{ component_id = "JAC2"; pin_name = "1" },
    [ordered]@{ component_id = "D2"; pin_name = "A" },
    [ordered]@{ component_id = "D4"; pin_name = "K" }
  ) },
  [ordered]@{ id = "DC_POS"; members = @(
    [ordered]@{ component_id = "JDC1"; pin_name = "1" },
    [ordered]@{ component_id = "D1"; pin_name = "K" },
    [ordered]@{ component_id = "D2"; pin_name = "K" },
    [ordered]@{ component_id = "C1"; pin_name = "1" },
    [ordered]@{ component_id = "RLOAD"; pin_name = "1" }
  ) },
  [ordered]@{ id = "DC_NEG"; members = @(
    [ordered]@{ component_id = "JDC2"; pin_name = "1" },
    [ordered]@{ component_id = "D3"; pin_name = "A" },
    [ordered]@{ component_id = "D4"; pin_name = "A" },
    [ordered]@{ component_id = "C1"; pin_name = "2" },
    [ordered]@{ component_id = "RLOAD"; pin_name = "2" }
  ) }
)
$ProjectJson = $ProjectObject | ConvertTo-Json -Depth 32
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Project, $ProjectJson, $Utf8NoBom)

Invoke-Ccad pcb add-route-request --file $Project --id RR_POS --net DC_POS --from D1.K --to C1.1 --preferred-layer F.Cu --policy bridge_positive_bus --width-mm 0.25
Invoke-Ccad pcb add-route-request --file $Project --id RR_NEG --net DC_NEG --from D3.A --to C1.2 --preferred-layer F.Cu --policy bridge_negative_bus --width-mm 0.25
Invoke-Ccad pcb export-route-job --file $Project --request-id RR_POS | Set-Content -Encoding UTF8 $RouteJob
Invoke-Ccad pcb apply-route-polyline --file $Project --request-id RR_POS --track-prefix RTPOS --points-mm "18,10;21,10;25,10" --complete true
Invoke-Ccad pcb apply-route-polyline --file $Project --request-id RR_NEG --track-prefix RTNEG --points-mm "18,24;21,24;25,24" --complete true
Invoke-Ccad pcb route-status --file $Project | Set-Content -Encoding UTF8 $RouteStatus
Invoke-Ccad pcb add-placement-region --file $Project --id PR1 --kind component --x-mm 11 --y-mm 4 --width-mm 12 --height-mm 8
Invoke-Ccad pcb add-keepout --file $Project --id K1 --kind placement --x-mm 38 --y-mm 26 --width-mm 4 --height-mm 3

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
Write-Output "Route job: $RouteJob"
Write-Output "Route status: $RouteStatus"
Write-Output "KiCad footprint: $KiCadFootprint"
Write-Output "Imported footprint: $ImportedFootprint"
Write-Output "Screenshot: $Screenshot"
