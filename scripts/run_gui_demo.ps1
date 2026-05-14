param(
  [string]$QtRoot = "C:\Qt\6.11.1\mingw_64",
  [string]$BuildDir = "build-qt",
  [string]$ProjectName = "gui-demo"
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repo $BuildDir
$projectPath = Join-Path $buildPath "$ProjectName.ccad.json"

cmake -S $repo -B $buildPath -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=ON "-DCMAKE_PREFIX_PATH=$QtRoot"
cmake --build $buildPath

& (Join-Path $buildPath "ccad.exe") init --name $ProjectName --out $projectPath

$env:PATH = (Join-Path $QtRoot "bin") + ";" + $env:PATH
Start-Process -FilePath (Join-Path $buildPath "ccad_gui.exe") -ArgumentList $projectPath

