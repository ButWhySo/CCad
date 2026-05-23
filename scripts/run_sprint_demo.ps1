param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$Name = "sprint-demo",
  [switch]$ClickSelection
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

function Invoke-CcadDrcReport {
  $Output = & $Ccad drc $Project
  $ExitCode = $LASTEXITCODE
  $Output | Set-Content -Encoding UTF8 $Drc
  if ($ExitCode -eq 2) {
    throw "ccad drc command failed with usage/file/parse error ($ExitCode): $Project"
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
Invoke-Ccad pcb add-keepout --file $Project --id K1 --kind placement --x-mm 20 --y-mm 10 --width-mm 4 --height-mm 3
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

$GuiOutput = & $Gui --screenshot $Project $Screenshot
$GuiExitCode = $LASTEXITCODE
if ($GuiExitCode -ne 0) {
  throw "ccad_gui screenshot failed ($GuiExitCode): $GuiOutput"
}
if (-not (Test-Path $Screenshot)) {
  throw "ccad_gui screenshot did not create: $Screenshot"
}

Write-Output "Project: $Project"
Write-Output "Inspect: $Inspect"
Write-Output "Validate: $Validate"
Write-Output "DRC: $Drc"
Write-Output "KiCad footprint: $KiCadFootprint"
Write-Output "Imported footprint: $ImportedFootprint"
Write-Output "Screenshot: $Screenshot"
