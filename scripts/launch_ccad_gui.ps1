param(
  [string]$Project = "artifacts/demos/sprint-demo.ccad.json",
  [switch]$ServeUiMap,
  [string]$ServerName = "",
  [string]$OutputDirectory = "",
  [switch]$KeepRunning
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

$runDirectory = if ($OutputDirectory) {
  if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $repo $OutputDirectory }
} else { Join-Path $repo "artifacts\\gui-launch" }
New-Item -ItemType Directory -Force -Path $runDirectory | Out-Null
$stdout = Join-Path $runDirectory "gui_stdout.log"
$stderr = Join-Path $runDirectory "gui_stderr.log"
$ready = Join-Path $runDirectory "ui_ready.txt"
if (Test-Path -LiteralPath $ready) { Remove-Item -LiteralPath $ready -Force }

[console]::Beep(800, 300)
Start-Sleep -Seconds 2
$arguments = @()
if ($ServeUiMap) {
  $pipe = if ($ServerName) { $ServerName } else { "ccad_agent_pipe_$PID" }
  $arguments += "--serve-ui-map", $projectPath, $pipe, $ready
} else { $arguments += $projectPath }

$process = Start-Process -FilePath $gui -ArgumentList $arguments -WindowStyle Maximized -PassThru `
  -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Start-Sleep -Seconds 7
$shot = Join-Path $runDirectory "00-launch.png"
if ($ServeUiMap -and -not $process.HasExited -and (Test-Path -LiteralPath $ready)) {
  & python (Join-Path $repo "scripts\\ccad_live_call.py") ui.screenshot --server $pipe --arg "path=$shot" |
    Out-File (Join-Path $runDirectory "00-launch.json")
}
[pscustomobject]@{
  process_id = $process.Id
  server = if ($ServeUiMap) { $pipe } else { "" }
  ready_file = $ready
  stdout = $stdout
  stderr = $stderr
  screenshot = if (Test-Path -LiteralPath $shot) { $shot } else { "" }
} | ConvertTo-Json -Compress
if ($KeepRunning) { Wait-Process -Id $process.Id }
