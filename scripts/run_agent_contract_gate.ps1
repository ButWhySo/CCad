param(
    [switch]$IncludeCheckpointRestart
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$python = Join-Path $root "src\ccad_agent\venv\Scripts\python.exe"
if (-not (Test-Path -LiteralPath $python)) {
    throw "Bundled agent venv missing: $python"
}

# Deliberately exclude GUI/visual/live-provider tests. This gate is offline and
# quota-safe; visual validation remains a separate user-authorized phase.
$excluded = @("test_gui_", "_gui_", "visual", "screenshot", "provider_real", "live")
$tests = Get-ChildItem -LiteralPath (Join-Path $root "scripts") -Filter "test_*.py" |
    Where-Object {
        $testName = $_.Name.ToLowerInvariant()
        -not ($excluded | Where-Object { $testName.Contains($_) })
    }
if (-not $IncludeCheckpointRestart) {
    $tests = $tests | Where-Object { $_.Name -ne "test_agent_checkpoint_restart.py" }
}

$failed = @()
foreach ($test in $tests) {
    & $python $test.FullName
    if ($LASTEXITCODE -ne 0) { $failed += $test.Name }
}
if ($failed.Count -gt 0) {
    throw "Agent contract gate failed: $($failed -join ', ')"
}
Write-Output "PASS bundled-venv offline agent gate: $($tests.Count) scripts"
