param(
  [string]$BuildDir = "build-qt",
  [string]$QtBin = "C:\Qt\6.11.1\mingw_64\bin",
  [string]$ProjectPath,
  [string]$Name = "ui-map-mouse-targets",
  [int]$InitialLoadMilliseconds = 5000,
  [int]$PerTargetMilliseconds = 800,
  [switch]$RequireNativeToolCatalog
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildPath = Join-Path $Root $BuildDir
$Artifacts = Join-Path $Root "artifacts"
$ScreenshotDir = Join-Path $Artifacts "screenshots"
New-Item -ItemType Directory -Force -Path $ScreenshotDir | Out-Null

$env:PATH = "$QtBin;$env:PATH"

$Gui = Join-Path $BuildPath "ccad_gui.exe"
if (-not (Test-Path $Gui)) {
  throw "Missing GUI executable: $Gui"
}
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
  $ProjectPath = Join-Path $Root "artifacts\demos\sprint160-placement-crash-ci-final.ccad.json"
}
if (-not (Test-Path $ProjectPath)) {
  throw "Missing project path: $ProjectPath"
}

function Invoke-PreTestBeep {
  $beepPath = Join-Path $Root "docs\beep.mp3"
  if (-not (Test-Path $beepPath)) {
    return
  }
  Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class CcadTargetWinMM {
  [DllImport("winmm.dll", CharSet = CharSet.Auto)]
  public static extern int mciSendString(string command, System.Text.StringBuilder buffer, int bufferSize, IntPtr hwndCallback);
}
"@
  [void][CcadTargetWinMM]::mciSendString("close ccad_target_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadTargetWinMM]::mciSendString("open `"$beepPath`" type mpegvideo alias ccad_target_beep", $null, 0, [IntPtr]::Zero)
  [void][CcadTargetWinMM]::mciSendString("play ccad_target_beep", $null, 0, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 350
  [void][CcadTargetWinMM]::mciSendString("close ccad_target_beep", $null, 0, [IntPtr]::Zero)
}

$stdoutLog = Join-Path $ScreenshotDir "$Name.stdout.log"
$stderrLog = Join-Path $ScreenshotDir "$Name.stderr.log"

Invoke-PreTestBeep
Start-Sleep -Seconds 2

$priorThreshold = $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS
$priorTraceDebug = $env:CCAD_TRACE_DEBUG
$priorAppData = $env:APPDATA
$priorMemoryPath = $env:CCAD_AGENT_MEMORY_PATH
$priorCheckpointPath = $env:CCAD_AGENT_CHECKPOINT_DB
$priorThreadId = $env:CCAD_AGENT_THREAD_ID
$priorDeferProvider = $env:CCAD_AGENT_DEFER_PROVIDER_INIT
$priorConversationDb = $env:CCAD_AGENT_CONVERSATION_DB
$isolatedMemoryProfile = $null
if ($Name.StartsWith("sprint969-context")) {
  # Exercise the real large-context branch with a deliberately low, valid
  # threshold. The /context feature remains local and never invokes the model.
  $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS = "512"
  $env:CCAD_TRACE_DEBUG = "1"
}
if ($Name.StartsWith("sprint971-memory")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint971-" + [Guid]::NewGuid().ToString("N"))
  New-Item -ItemType Directory -Path $isolatedMemoryProfile | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
}
if ($Name.StartsWith("sprint974-memory")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint974-" + [Guid]::NewGuid().ToString("N"))
  New-Item -ItemType Directory -Path $isolatedMemoryProfile | Out-Null
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "sprint974-test-thread"
  $env:CCAD_AGENT_DEFER_PROVIDER_INIT = "1"
  $testConfig = [ordered]@{
    provider = "openai"
    model = "gpt-5.1"
    memory = @{ stm = $false; ltm = $true; episodic = $false }
    observability = @{ enabled = $false; backend = "langfuse"; environment = "development" }
  }
  [IO.File]::WriteAllText((Join-Path $configDir "agent_config.json"),
    ($testConfig | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
  $fixedText = "Preserve verified connector J3 placement and the existing ground return path. Keep minimum copper clearance at 0.25 mm on F.Cu; do not move U3 or alter net assignments. This durable preference is retained only to validate explicit memory-compaction planning and cancellation in an isolated test profile."
  $testRecords = @(
    @{ id = "sprint974-memory-a"; tier = "ltm"; namespace = "sprint974-test-thread"; scope = "conversation"; title = "PCB constraints A"; content = $fixedText; tags = @("pcb", "clearance"); created_at = "2026-09-20T10:00:00+00:00"; expires_at = "" },
    @{ id = "sprint974-memory-b"; tier = "ltm"; namespace = "sprint974-test-thread"; scope = "conversation"; title = "PCB constraints B"; content = ($fixedText + " Also preserve the current board outline and all existing via locations during unrelated edits."); tags = @("pcb", "outline"); created_at = "2026-09-21T10:00:00+00:00"; expires_at = "" }
  )
  [IO.File]::WriteAllText($env:CCAD_AGENT_MEMORY_PATH,
    (ConvertTo-Json -InputObject $testRecords -Depth 8), [Text.UTF8Encoding]::new($false))
  $script:memoryBeforeHash = (Get-FileHash -LiteralPath $env:CCAD_AGENT_MEMORY_PATH -Algorithm SHA256).Hash
}
if ($Name.StartsWith("sprint975-memory-ui")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint975-" + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "sprint975-memory-ui-thread"
  $env:CCAD_AGENT_DEFER_PROVIDER_INIT = "1"
  $testConfig = [ordered]@{
    provider = "openai"
    model = "gpt-5.1"
    memory = @{ stm = $false; ltm = $true; episodic = $false }
    observability = @{ enabled = $false; backend = "langfuse"; environment = "development" }
  }
  [IO.File]::WriteAllText((Join-Path $configDir "agent_config.json"),
    ($testConfig | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
}
if ($Name.StartsWith("sprint977-context")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint977-context-" + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CONVERSATION_DB = Join-Path $isolatedMemoryProfile "agent_conversations.sqlite3"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "sprint977-context-ui-thread"
  $env:CCAD_AGENT_DEFER_PROVIDER_INIT = "1"
  $testConfig = [ordered]@{
    provider = "openai"
    model = "gpt-5.1"
    memory = @{ stm = $false; ltm = $true; episodic = $false }
    observability = @{ enabled = $false; backend = "langfuse"; environment = "development" }
  }
  [IO.File]::WriteAllText((Join-Path $configDir "agent_config.json"),
    ($testConfig | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
  $testMemory = @(@{
    id = "sprint977-gnd-u3-memory"
    title = "GND routing near U3"
    content = "Keep the GND return path short around U3 on F.Cu."
    scope = "conversation"
    tier = "ltm"
    namespace = $env:CCAD_AGENT_THREAD_ID
    tags = @("GND", "U3", "F.Cu")
    created_at = [DateTime]::UtcNow.ToString("o")
    expires_at = ""
  })
  [IO.File]::WriteAllText($env:CCAD_AGENT_MEMORY_PATH,
    (ConvertTo-Json -InputObject $testMemory -Depth 8), [Text.UTF8Encoding]::new($false))
}
if ($Name.StartsWith("sprint976-conversation")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint976-" + [Guid]::NewGuid().ToString("N"))
  New-Item -ItemType Directory -Path $isolatedMemoryProfile -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_CONVERSATION_DB = Join-Path $isolatedMemoryProfile "agent_conversations.sqlite3"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "sprint976-conversation-ui-thread"
  $env:CCAD_AGENT_DEFER_PROVIDER_INIT = "1"
}
if ($Name.StartsWith("sprint980-project-retrieval") -or
    $Name.StartsWith("sprint981-schematic-project-graph")) {
  $profilePrefix = if ($Name.StartsWith("sprint981-schematic-project-graph")) {
    "ccad-sprint981-project-graph-"
  } else { "ccad-sprint980-project-" }
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ($profilePrefix + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_CONVERSATION_DB = Join-Path $isolatedMemoryProfile "agent_conversations.sqlite3"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = if ($Name.StartsWith("sprint981-schematic-project-graph")) {
    "sprint981-schematic-project-graph-ui-thread"
  } else { "sprint980-project-retrieval-ui-thread" }
  $env:CCAD_AGENT_DEFER_PROVIDER_INIT = "1"
  $testConfig = [ordered]@{
    provider = "openai"
    model = "gpt-5.1"
    memory = @{ stm = $false; ltm = $false; episodic = $false }
    observability = @{ enabled = $false; backend = "langfuse"; environment = "development" }
  }
  [IO.File]::WriteAllText((Join-Path $configDir "agent_config.json"),
    ($testConfig | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
}
try {
  $process = Start-Process -FilePath $Gui -WindowStyle Maximized `
    -ArgumentList @("--test-ui-map-target-sequence", $ProjectPath, $ScreenshotDir, $Name,
                    [string]$InitialLoadMilliseconds, [string]$PerTargetMilliseconds) `
    -PassThru -Wait -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
  if ($Name.StartsWith("sprint971-memory")) {
    $memoryFile = Join-Path $isolatedMemoryProfile "agent_memory.json"
    $configFile = Join-Path $isolatedMemoryProfile "CCad\agent_config.json"
    if (Test-Path -LiteralPath $configFile) {
      $config = Get-Content -Raw -LiteralPath $configFile | ConvertFrom-Json
      $safePreferences = [ordered]@{
        stm = [bool]$config.memory.stm
        ltm = [bool]$config.memory.ltm
        episodic = [bool]$config.memory.episodic
      }
      $safePreferences | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ScreenshotDir "$Name-memory-preferences.json")
    } else {
      throw "Mapped Agent run did not create its isolated configuration file."
    }
    if (-not (Test-Path -LiteralPath $memoryFile)) {
      throw "Mapped memory flow did not create its isolated durable memory store."
    }
    $records = @(Get-Content -Raw -LiteralPath $memoryFile | ConvertFrom-Json)
    $savedUiRecord = $records | Where-Object {
      $_.tier -eq "ltm" -and $_.title -eq "UI-map verification" -and
      $_.scope -eq "conversation" -and
      $_.content -eq "Sprint 971 UI-map proof record; safe to delete"
    } | Select-Object -First 1
    if (-not $savedUiRecord) {
      throw "Mapped GUI did not persist the exact LTM title, scope, and content."
    }
    $confirmationScreenshot = Join-Path $ScreenshotDir "$Name-initial-reset-confirmation.png"
    if (-not (Test-Path -LiteralPath $confirmationScreenshot)) {
      throw "Mapped GUI did not capture the reset confirmation before cancelling it."
    }
  }
  if ($Name.StartsWith("sprint974-memory")) {
    $memoryAfterHash = (Get-FileHash -LiteralPath $env:CCAD_AGENT_MEMORY_PATH -Algorithm SHA256).Hash
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    if ($memoryBeforeHash -ne $memoryAfterHash) {
      throw "Plan/cancel GUI flow unexpectedly changed persistent memory."
    }
    if (-not ($reportData.entries | Where-Object { $_.plan_created -eq $true }) -or
        -not ($reportData.entries | Where-Object { $_.plan_cancelled -eq $true })) {
      throw "GUI did not prove explicit durable-memory compaction plan and cancellation."
    }
    if ($stdoutLog -and (Select-String -LiteralPath $stdoutLog -Pattern '"provider_request_sent":true' -Quiet)) {
      throw "Provider request occurred during plan/cancel validation."
    }
  }
  if ($Name.StartsWith("sprint975-memory-ui")) {
    $memoryFile = $env:CCAD_AGENT_MEMORY_PATH
    if (-not (Test-Path -LiteralPath $memoryFile)) {
      throw "Mapped memory CRUD scenario did not create the isolated durable store."
    }
    $memoryJson = [IO.File]::ReadAllText($memoryFile)
    $records = ConvertFrom-Json -InputObject $memoryJson
    if ($records.Count -ne 0) {
      throw "Mapped memory CRUD scenario left a record after confirmed deletion: $memoryJson"
    }
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    foreach ($field in @("memory_empty_write_rejected", "memory_added", "memory_updated", "memory_deleted")) {
      if (-not ($reportData.entries | Where-Object { $_.$field -eq $true })) {
        throw "Mapped memory CRUD scenario did not verify '$field'. Report: $reportPath"
      }
    }
    $confirmation = Join-Path $ScreenshotDir "$Name-memory-ui-delete-confirmation.png"
    if (-not (Test-Path -LiteralPath $confirmation)) {
      throw "Mapped memory deletion did not capture its confirmation dialog."
    }
  }
  if ($Name.StartsWith("sprint976-conversation") -or $Name.StartsWith("sprint977-context") -or
      $Name.StartsWith("sprint980-project-retrieval") -or
      $Name.StartsWith("sprint981-schematic-project-graph")) {
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    foreach ($field in @("conversation_turn_visible", "canonical_transcript_retained_after_clear")) {
      if (-not ($reportData.entries | Where-Object { $_.$field -eq $true })) {
        throw "Mapped conversation scenario did not verify '$field'. Report: $reportPath"
      }
    }
    if (-not (Test-Path -LiteralPath $env:CCAD_AGENT_CONVERSATION_DB)) {
      throw "GUI run did not create its isolated durable conversation database."
    }
    $agentPython = Join-Path $Root "src\ccad_agent\venv\Scripts\python.exe"
    if (-not (Test-Path -LiteralPath $agentPython)) {
      throw "The configured Agent Python runtime is required to inspect the conversation database."
    }
    $databaseVerifier = Join-Path $Root "scripts\verify_conversation_ui_state.py"
    $expectedThreadId = if ($Name.StartsWith("sprint981-schematic-project-graph")) {
      "sprint981-schematic-project-graph-ui-thread"
    } elseif ($Name.StartsWith("sprint980-project-retrieval")) {
      "sprint980-project-retrieval-ui-thread"
    } elseif ($Name.StartsWith("sprint977-context")) {
      "sprint977-context-ui-thread"
    } else {
      "sprint976-conversation-ui-thread"
    }
    $databaseStateJson = & $agentPython $databaseVerifier $env:CCAD_AGENT_CONVERSATION_DB $expectedThreadId
    if ($LASTEXITCODE -ne 0) { throw "Could not inspect the isolated conversation database." }
    $databaseState = $databaseStateJson | ConvertFrom-Json
    if ($databaseState.messages -ne 2 -or $databaseState.users -ne 1 -or
        $databaseState.assistants -ne 1 -or $databaseState.turn_records -ne 1 -or
        $databaseState.projection -ne "[]") {
      throw "Unexpected persisted transcript/projection state: $databaseStateJson"
    }
    if ($Name.StartsWith("sprint977-context") -and
        -not ($reportData.entries | Where-Object { $_.context_memory_attached -eq $true })) {
      throw "Mapped turn did not visibly prove inclusion of its enabled scoped memory entry."
    }
    if (($Name.StartsWith("sprint980-project-retrieval") -or
         $Name.StartsWith("sprint981-schematic-project-graph")) -and
        -not ($reportData.entries | Where-Object { $_.project_retrieval_visible -eq $true })) {
      throw "Mapped turn did not visibly prove a non-empty typed-project retrieval result."
    }
    $databaseState | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ScreenshotDir "$Name-database-verification.json")
    if ($stdoutLog -and (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet)) {
      throw "Provider request occurred during conversation UI validation."
    }
    if ($Name.StartsWith("sprint981-schematic-project-graph") -and
        (-not ($reportData.entries | Where-Object { $_.schematic_pin_retrieval_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.schematic_symbol_retrieval_visible -eq $true }))) {
      throw "Mapped turn did not visibly include retrieved schematic net pins and related symbols."
    }
    $requiredScreenshots = if ($Name.StartsWith("sprint981-schematic-project-graph")) {
      @("before", "turn-persisted", "projection-cleared")
    } elseif ($Name.StartsWith("sprint980-project-retrieval")) {
      @("before", "turn-persisted")
    } else { @("before", "turn-persisted", "projection-cleared") }
    foreach ($state in $requiredScreenshots) {
      if (-not (Test-Path -LiteralPath (Join-Path $ScreenshotDir "$Name-$state.png"))) {
        throw "Conversation UI validation is missing the '$state' visual checkpoint."
      }
    }
  }
} finally {
  if ($null -eq $priorThreshold) { Remove-Item Env:CCAD_AGENT_LARGE_CONTEXT_TOKENS -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_LARGE_CONTEXT_TOKENS = $priorThreshold }
  if ($null -eq $priorTraceDebug) { Remove-Item Env:CCAD_TRACE_DEBUG -ErrorAction SilentlyContinue }
  else { $env:CCAD_TRACE_DEBUG = $priorTraceDebug }
  if ($null -eq $priorAppData) { Remove-Item Env:APPDATA -ErrorAction SilentlyContinue }
  else { $env:APPDATA = $priorAppData }
  if ($null -eq $priorMemoryPath) { Remove-Item Env:CCAD_AGENT_MEMORY_PATH -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_MEMORY_PATH = $priorMemoryPath }
  if ($null -eq $priorCheckpointPath) { Remove-Item Env:CCAD_AGENT_CHECKPOINT_DB -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_CHECKPOINT_DB = $priorCheckpointPath }
  if ($null -eq $priorThreadId) { Remove-Item Env:CCAD_AGENT_THREAD_ID -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_THREAD_ID = $priorThreadId }
  if ($null -eq $priorDeferProvider) { Remove-Item Env:CCAD_AGENT_DEFER_PROVIDER_INIT -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_DEFER_PROVIDER_INIT = $priorDeferProvider }
  if ($null -eq $priorConversationDb) { Remove-Item Env:CCAD_AGENT_CONVERSATION_DB -ErrorAction SilentlyContinue }
  else { $env:CCAD_AGENT_CONVERSATION_DB = $priorConversationDb }
  if ($isolatedMemoryProfile -and (Test-Path -LiteralPath $isolatedMemoryProfile)) {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $profilePath = [IO.Path]::GetFullPath($isolatedMemoryProfile)
    if (-not $profilePath.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
      throw "Refusing to remove a memory-test profile outside the system temp directory."
    }
    Remove-Item -LiteralPath $profilePath -Recurse -Force
  }
}

if ($process.ExitCode -ne 0) {
  throw "UI map target sequence failed with exit code $($process.ExitCode). Stdout: $stdoutLog Stderr: $stderrLog"
}

$Report = Join-Path $ScreenshotDir "$Name-target-sequence.json"
$reportData = Get-Content -Raw $Report | ConvertFrom-Json
if ($Name.StartsWith("sprint972-provider")) {
  $readyEvent = @($reportData.entries | Where-Object {
    $_.provider_local_validation_ready -eq $true
  }) | Select-Object -First 1
  $clickedLocalTest = @($reportData.entries | Where-Object {
    $_.id -eq "action:testProviderBtn" -and $_.interaction -eq "ui.click" -and
    $_.result.result.performed -eq $true
  }) | Select-Object -First 1
  $clickedCancel = @($reportData.entries | Where-Object {
    $_.id -eq "action:cancelSettingsButton" -and $_.interaction -eq "ui.click" -and
    $_.result.result.performed -eq $true
  }) | Select-Object -First 1
  if (-not $readyEvent -or -not $clickedLocalTest -or -not $clickedCancel) {
    throw "Provider UI validation did not prove local no-network readiness and clean dialog close. Report: $Report"
  }
  if (@($reportData.entries | Where-Object { $_.id -eq "action:testProviderConnectionBtn" }).Count -gt 0) {
    throw "Provider validation unexpectedly targeted the quota-consuming live connection test."
  }
}
if ($RequireNativeToolCatalog) {
  if (-not $reportData.catalog_startup_verified) {
    throw "Native agent tool catalog was not installed before persisted provider activation. Report: $Report"
  }
}
Write-Output "Report: $Report"
Write-Output "Stdout: $stdoutLog"
Write-Output "Stderr: $stderrLog"
