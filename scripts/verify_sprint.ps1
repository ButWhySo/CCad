param(
  [Parameter(Mandatory)][ValidatePattern('^[a-z0-9][a-z0-9-]{2,63}$')][string]$SprintId,
  [Parameter(Mandatory)][string]$FeatureDescription,
  [string]$InteractionPlan,
  [string]$ReusePassedBuildAndTestsFrom,
  [switch]$NonVisual,
  [switch]$WorkspaceOnlyEvidence
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Set-Location $root
$plan = $null
$planPath = $null
if ($NonVisual) {
  if ($InteractionPlan) { throw '-InteractionPlan cannot be combined with -NonVisual.' }
  $validationMode = 'non_visual'
  $runName = "$SprintId-nonvisual"
} else {
  if (-not $InteractionPlan) { throw 'GUI validation requires -InteractionPlan; use -NonVisual only when no GUI behavior changed.' }
  $planPath = (Resolve-Path -LiteralPath $InteractionPlan).Path
  $plan = Get-Content -Raw -LiteralPath $planPath | ConvertFrom-Json
  if (-not $plan.target_sequence -or -not $plan.project -or
      $plan.minimum_mapped_interactions -lt 7 -or
      $plan.required_targets.Count -lt 7 -or
      $plan.meaningful_screenshots -lt 1) {
    throw 'Plan must name an existing app-owned target sequence, project, >=7 mapped targets, and expected checkpoint count.'
  }
  $validationMode = 'gui'
  $runName = "$($plan.target_sequence)-$SprintId"
}
$evidenceRoot = Join-Path $root "artifacts\evidence\$SprintId"
$manifestPath = Join-Path $root "artifacts\evidence\$SprintId.json"
$screenshotsRoot = Join-Path $root 'artifacts\screenshots'
New-Item -ItemType Directory -Force -Path $evidenceRoot | Out-Null
if (Test-Path -LiteralPath $manifestPath) {
  throw "Refusing to overwrite existing evidence manifest: $manifestPath"
}
$manifest = [ordered]@{
  schema_version = 1
  sprint_id = $SprintId
  feature = $FeatureDescription
  validation_mode = $validationMode
  interaction_plan = if ($planPath) { $planPath.Substring($root.Length + 1).Replace('\','/') } else { $null }
  target_sequence = if ($plan) { $runName } else { $null }
  visual_validation = if ($NonVisual) { 'not_applicable_no_gui_behavior_changed' } else { 'required' }
  started_utc = [DateTime]::UtcNow.ToString('o')
  status = 'in_progress'
  steps = @()
  artifacts = @()
  workspace_only_artifacts = @()
}
function Add-Step([string]$Name, [bool]$Passed, [string]$Detail) {
  $manifest.steps += [ordered]@{
    name = $Name; passed = $Passed; detail = $Detail
    finished_utc = [DateTime]::UtcNow.ToString('o')
  }
  if (-not $Passed) { throw "Verification gate failed: $Name - $Detail" }
}
function Invoke-Logged([string]$Name, [scriptblock]$Command, [string]$LogPath) {
  $started = [DateTime]::UtcNow
  & $Command 2>&1 | Tee-Object -FilePath $LogPath
  $exit = $LASTEXITCODE
  $duration = ([DateTime]::UtcNow - $started).TotalSeconds
  Add-Step $Name ($exit -eq 0) "exit=$exit duration_seconds=$([Math]::Round($duration,2)) log=$($LogPath.Substring($root.Length+1).Replace('\','/'))"
}

try {
  $preflightLog = Join-Path $evidenceRoot 'preflight.log'
  Invoke-Logged 'qt_mingw_preflight' {
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/preflight_qt_env.ps1
  } $preflightLog

  $env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
  if ($ReusePassedBuildAndTestsFrom) {
    $priorRoot = Join-Path $root "artifacts\evidence\$ReusePassedBuildAndTestsFrom"
    $priorManifest = Join-Path $root "artifacts\evidence\$ReusePassedBuildAndTestsFrom.json"
    if (-not (Test-Path -LiteralPath $priorManifest)) { throw "Prior gate manifest missing: $priorManifest" }
    $prior = Get-Content -Raw -LiteralPath $priorManifest | ConvertFrom-Json
    foreach ($stepName in @('release_build','full_ctest')) {
      if (-not ($prior.steps | Where-Object { $_.name -eq $stepName -and $_.passed -eq $true })) {
        throw "Prior gate does not contain a successful $stepName step."
      }
    }
    $priorBuild = Join-Path $priorRoot 'build.log'
    $priorTests = Join-Path $priorRoot 'ctest.log'
    if (-not (Test-Path -LiteralPath $priorBuild) -or
        -not (Select-String -LiteralPath $priorTests -Pattern '100% tests passed, 0 tests failed out of 115' -Quiet)) {
      throw 'Prior build/test logs do not prove the expected successful full suite.'
    }
    $codeFiles = @(Get-ChildItem src,tests -Recurse -File | Where-Object {
      $_.Extension -in @('.cpp','.hpp','.h','.py')
    }) + @(Get-Item CMakeLists.txt)
    $latestCodeWrite = ($codeFiles | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
    if ($latestCodeWrite -gt (Get-Item -LiteralPath $priorTests).LastWriteTimeUtc) {
      throw 'Source or test files changed after the prior complete CTest run; do not reuse it.'
    }
    Copy-Item -LiteralPath $priorBuild -Destination (Join-Path $evidenceRoot 'build.log')
    Copy-Item -LiteralPath $priorTests -Destination (Join-Path $evidenceRoot 'ctest.log')
    Add-Step 'release_build' $true "reused verified prior run $ReusePassedBuildAndTestsFrom; source timestamps precede test log"
    Add-Step 'full_ctest' $true "reused verified prior run $ReusePassedBuildAndTestsFrom; 115/115 passed"
  } else {
    Invoke-Logged 'release_build' {
      cmake --build build-qt --config Release --parallel 8
    } (Join-Path $evidenceRoot 'build.log')
    Invoke-Logged 'full_ctest' {
      ctest --test-dir build-qt --output-on-failure
    } (Join-Path $evidenceRoot 'ctest.log')
  }

  if ($NonVisual) {
    Write-Output 'Visual validation: not applicable; this verification run is explicitly non-visual.'
  } else {
    $project = Join-Path $root $plan.project
    if (-not (Test-Path -LiteralPath $project)) { throw "Plan project missing: $project" }
    Invoke-Logged 'official_ui_map_target_sequence' {
      powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/run_ui_map_mouse_target_demo.ps1 `
        -Name $runName -ProjectPath $project -InitialLoadMilliseconds 5000 -PerTargetMilliseconds 800
    } (Join-Path $evidenceRoot 'ui_harness.log')

  $report = Join-Path $screenshotsRoot "$runName-target-sequence.json"
  $stdout = Join-Path $screenshotsRoot "$runName.stdout.log"
  $stderr = Join-Path $screenshotsRoot "$runName.stderr.log"
  foreach ($required in @($report,$stdout,$stderr)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "UI harness evidence missing: $required" }
  }
  $reportData = Get-Content -Raw -LiteralPath $report | ConvertFrom-Json
  $entries = @($reportData.entries)
  $actions = @($entries | Where-Object {
    $_.interaction -in @('ui.click','ui.type_text') -and $_.result.result.performed -eq $true
  })
  if ($actions.Count -lt $plan.minimum_mapped_interactions) {
    throw "Only $($actions.Count) successful mapped interactions; required $($plan.minimum_mapped_interactions)."
  }
  foreach ($target in $plan.required_targets) {
    if (-not ($entries | Where-Object {
      $_.id -eq $target -or $_.target -eq $target -or
      $_.interaction -like "*:$target*" -or $_.target.id -eq $target
    })) {
      throw "Required mapped target absent from run report: $target"
    }
  }
  foreach ($target in $plan.required_text_entries) {
    if (-not ($entries | Where-Object {
      $_.interaction -eq 'ui.type_text' -and
      ($_.id -eq $target -or $_.target -eq $target) -and
      $_.result.result.performed -eq $true
    })) { throw "Required mapped text entry failed or missing: $target" }
  }
  $images = @(Get-ChildItem -LiteralPath $screenshotsRoot -Filter "$runName-*.png" -File)
  if ($images.Count -ne $plan.meaningful_screenshots) {
    throw "Expected $($plan.meaningful_screenshots) meaningful screenshots, found $($images.Count)."
  }
  if ((Get-Content -Raw -LiteralPath $stdout) -match 'provider_request_sent":true') {
    throw 'Provider request unexpectedly occurred during isolated UI validation.'
  }
  Add-Step 'mapped_interaction_contract' $true "successful_interactions=$($actions.Count); screenshots=$($images.Count)"

  foreach ($source in @($report,$stdout,$stderr) + @($images | ForEach-Object FullName)) {
    Copy-Item -LiteralPath $source -Destination $evidenceRoot
  }
  $stderrText = Get-Content -Raw -LiteralPath $stderr
  if ($stderrText -match '(?im)(QWidget::|QLayout::|QPainter::|ASSERT failure|segmentation fault|fatal error)') {
    throw 'GUI stderr contains a severe Qt/rendering/runtime failure; inspect the preserved log.'
  }
  Add-Step 'captured_gui_logs_and_images' $true 'stdout, stderr, action report, and every scoped PNG copied into evidence folder'
  }

  foreach ($textArtifact in Get-ChildItem -LiteralPath $evidenceRoot -File | Where-Object {
    $_.Extension -in @('.json','.log')
  }) {
    $text = [IO.File]::ReadAllText($textArtifact.FullName)
    $text = $text.Replace("`r`n", "`n").Replace("`r", "`n")
    [IO.File]::WriteAllText($textArtifact.FullName,$text,[Text.UTF8Encoding]::new($false))
  }
  $manifest.status = 'pass'
  $manifest.finished_utc = [DateTime]::UtcNow.ToString('o')
  $artifactDigests = @(Get-ChildItem -LiteralPath $evidenceRoot -File | Sort-Object Name | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($root.Length+1).Replace('\','/'); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
  })
  if ($WorkspaceOnlyEvidence) {
    $manifest.workspace_only_artifacts = $artifactDigests
  } else {
    $manifest.artifacts = $artifactDigests
  }
  $json = $manifest | ConvertTo-Json -Depth 8
  [IO.File]::WriteAllText($manifestPath,$json.Replace("`r`n", "`n").Replace("`r", "`n"),[Text.UTF8Encoding]::new($false))
  $manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
  Write-Output "PASS manifest=$($manifestPath.Substring($root.Length+1).Replace('\','/')) sha256=$manifestHash"
} catch {
  $manifest.status = 'fail'
  $manifest.failure = $_.Exception.Message
  $manifest.finished_utc = [DateTime]::UtcNow.ToString('o')
  [IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
  throw
}
