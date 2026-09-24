param(
  [string]$BuildDir = "build-qt",
  [string]$QtRoot = "C:\Qt\6.11.1\mingw_64",
  [string]$ExpectedCompiler = "C:\Qt\Tools\mingw1310_64\bin\g++.exe"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$qtBin = Join-Path $QtRoot "bin"
$cache = Join-Path $root "$BuildDir\CMakeCache.txt"

if (-not (Test-Path $QtRoot)) {
  throw "Qt root not found: $QtRoot"
}
if (-not (Test-Path $qtBin)) {
  throw "Qt bin not found: $qtBin"
}
if (-not (Test-Path $ExpectedCompiler)) {
  throw "Expected compiler not found: $ExpectedCompiler"
}

$env:PATH = "$qtBin;$env:PATH"

$qtDllPath = (& where.exe Qt6Core.dll 2>$null | Select-Object -First 1)
if (-not $qtDllPath) {
  throw "Qt6Core.dll not found in PATH."
}
$qtDllPath = $qtDllPath.Trim()
$qtDllResolved = (Resolve-Path $qtDllPath).Path
$expectedDll = Join-Path $qtBin "Qt6Core.dll"
$expectedDllResolved = (Resolve-Path $expectedDll).Path
if ($qtDllResolved -ne $expectedDllResolved) {
  throw "Wrong Qt6Core.dll resolved first: $qtDllResolved`nExpected: $expectedDllResolved"
}

$compilerResolved = (Resolve-Path $ExpectedCompiler).Path

if (Test-Path $cache) {
  $compilerLine = Select-String -Path $cache -Pattern '^CMAKE_CXX_COMPILER:(FILEPATH|STRING|UNINITIALIZED)=' | Select-Object -First 1
  if (-not $compilerLine) {
    throw "CMAKE_CXX_COMPILER missing in $cache"
  }
  $cachedCompiler = $compilerLine.Line.Split('=')[-1].Trim()
  if ($cachedCompiler) {
    $cachedResolved = (Resolve-Path $cachedCompiler).Path
    if ($cachedResolved -ne $compilerResolved) {
      throw "Toolchain mismatch in CMake cache.`nCached: $cachedResolved`nExpected: $compilerResolved"
    }
  }
}

Write-Output "Preflight OK"
Write-Output "Qt DLL: $qtDllResolved"
Write-Output "Compiler: $compilerResolved"
