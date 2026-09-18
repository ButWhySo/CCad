param(
  [string]$Project = "artifacts/demos/sprint-demo.ccad.json",
  [switch]$ServeUiMap
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$qtBin = "C:\Qt\6.11.1\mingw_64\bin"
$mingwBin = "C:\Qt\Tools\mingw1310_64\bin"
$env:PATH = "$qtBin;$mingwBin;$env:PATH"
$gui = Join-Path $repo "build-qt\ccad_gui.exe"
$projectPath = if ([IO.Path]::IsPathRooted($Project)) { $Project } else { Join-Path $repo $Project }

if (-not (Test-Path -LiteralPath $gui)) { throw "GUI not built: $gui" }
if (-not (Test-Path -LiteralPath $projectPath)) { throw "Project not found: $projectPath" }

if ($ServeUiMap) {
  $pipe = "ccad_agent_pipe_$PID"
  $ready = Join-Path $repo "ui_ready.txt"
  if (Test-Path -LiteralPath $ready) { Remove-Item -LiteralPath $ready -Force }
  & $gui --serve-ui-map $projectPath $pipe $ready
} else {
  & $gui $projectPath
}
