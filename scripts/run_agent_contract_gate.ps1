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
        ($testName -eq "test_mcp_gui_bridge.py") -or
            (-not ($excluded | Where-Object { $testName.Contains($_) }))
    }
if (-not $IncludeCheckpointRestart) {
    $tests = $tests | Where-Object { $_.Name -ne "test_agent_checkpoint_restart.py" }
} else {
    # This fixture is multi-phase and needs an explicit durable DB per branch;
    # do not feed it through the ordinary one-argument test loop.
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
if ($IncludeCheckpointRestart) {
    $checkpointTest = Join-Path $root "scripts\test_agent_checkpoint_restart.py"
    foreach ($decision in @("second", "denial", "cancel")) {
        $checkpointDb = Join-Path $env:TEMP ("ccad-checkpoint-" + [guid]::NewGuid().ToString("N") + ".sqlite")
        try {
            $env:CCAD_RESTART_PHASE = "first"
            & $python $checkpointTest $checkpointDb
            if ($LASTEXITCODE -ne 0) { throw "checkpoint first phase failed ($decision)" }
            $env:CCAD_RESTART_PHASE = $decision
            & $python $checkpointTest $checkpointDb
            if ($LASTEXITCODE -ne 0) { throw "checkpoint $decision phase failed" }
        } finally {
            Remove-Item -LiteralPath $checkpointDb -Force -ErrorAction SilentlyContinue
            Remove-Item Env:CCAD_RESTART_PHASE -ErrorAction SilentlyContinue
        }
    }
    Write-Output "PASS checkpoint restart branches: accept, denial, cancel"
}
Write-Output "PASS bundled-venv offline agent gate: $($tests.Count) scripts"
