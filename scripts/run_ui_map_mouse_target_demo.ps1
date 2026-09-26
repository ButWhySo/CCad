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
$isolatedProjectPath = $null
if ($Name.StartsWith("sprint982-multilayer-project-context") -or
    $Name.StartsWith("sprint983-project-index-typed-geometry") -or
    $Name.StartsWith("sprint985-project-reference-graph") -or
    $Name.StartsWith("sprint986-project-spatial-index") -or
    $Name.StartsWith("sprint987-schematic-metadata") -or
    $Name.StartsWith("sprint998-functional-block-net-context")) {
  $isolatedProjectPath = Join-Path ([IO.Path]::GetTempPath()) (
    "ccad-sprint" + $(if ($Name.StartsWith("sprint998")) { "998-block-net-" } elseif ($Name.StartsWith("sprint987")) { "987-schematic-metadata-" } elseif ($Name.StartsWith("sprint986")) { "986-project-spatial-" } elseif ($Name.StartsWith("sprint985")) { "985-project-graph-" } elseif ($Name.StartsWith("sprint983")) { "983-typed-geometry-" } else { "982-multilayer-" }) +
    [Guid]::NewGuid().ToString("N") + ".ccad.json")
  Copy-Item -LiteralPath $ProjectPath -Destination $isolatedProjectPath
  if ($Name.StartsWith("sprint983-project-index-typed-geometry")) {
    $fixture = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
    if (-not $fixture.board.pads -or $fixture.board.pads.Count -eq 0) {
      throw "Typed layer GUI scenario requires a serialized board pad."
    }
    $fixture.board.pads[0] | Add-Member -MemberType NoteProperty -Name padstack `
      -Value ([pscustomobject]@{ layer_set = @("F.Cu", "B.Cu") }) -Force
    [IO.File]::WriteAllText($isolatedProjectPath,
      (ConvertTo-Json -InputObject $fixture -Depth 64), [Text.UTF8Encoding]::new($false))
  } elseif ($Name.StartsWith("sprint987-schematic-metadata")) {
    $fixture = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
    if (-not $fixture.components -or $fixture.components.Count -eq 0) {
      throw "Schematic metadata scenario requires a serialized schematic component."
    }
    $fixture.components[0] | Add-Member -MemberType NoteProperty -Name reference `
      -Value "U3" -Force
    if ($Name.Contains("sprint992")) {
      $fixture.components[0] | Add-Member -MemberType NoteProperty -Name unit `
        -Value 1 -Force
      $fixture.components[0] | Add-Member -MemberType NoteProperty -Name pins -Value @([pscustomobject]@{
        name = "PGOOD"
        number = "2"
        electrical_type = "output"
        orientation = "right"
      }, [pscustomobject]@{
        name = "GND"
        number = "3"
        electrical_type = "power_in"
        orientation = "left"
      }) -Force
    }
    $fixture.components[0] | Add-Member -MemberType NoteProperty -Name fields -Value @([pscustomobject]@{
      id = "FIELD_SPRINT987_MANUFACTURER"
      name = "Manufacturer"
      text = "ACME-42"
      position = [pscustomobject]@{ x_nm = 0; y_nm = 0 }
      rotation_degrees = 0
      size = [pscustomobject]@{ width_nm = 1000000; height_nm = 1000000 }
      visible = $true
    }, [pscustomobject]@{
      id = "FIELD_SPRINT987_ORDER_CODE"
      name = "Order code"
      text = "ACME-42"
      position = [pscustomobject]@{ x_nm = 2000000; y_nm = 0 }
      rotation_degrees = 0
      size = [pscustomobject]@{ width_nm = 1000000; height_nm = 1000000 }
      visible = $true
    }) -Force
    $fixture | Add-Member -MemberType NoteProperty -Name sheets -Value @([pscustomobject]@{
      id = "SHEET_SPRINT987"
      name = "Power Stage"
      file_path = "sheets/power_stage.kicad_sch"
      position = [pscustomobject]@{ x_nm = 0; y_nm = 0 }
      size = [pscustomobject]@{ width_nm = 40000000; height_nm = 30000000 }
      pins = @()
    }) -Force
    if ($Name.Contains("sprint997")) {
      if (-not $fixture.board.footprints) {
        $fixture.board | Add-Member -MemberType NoteProperty -Name footprints -Value @() -Force
      }
      if (-not $fixture.board.placement_regions) {
        $fixture.board | Add-Member -MemberType NoteProperty -Name placement_regions -Value @() -Force
      }
      $fixture.board.footprints += [pscustomobject]@{
        reference = "JAC1"
        value = "AC input"
        footprint_name = "Connector_PinHeader_2.54mm"
        layer_id = "F.Cu"
        position = [pscustomobject]@{ x_nm = 8000000; y_nm = 17000000 }
      }
      $fixture.board.footprints += [pscustomobject]@{
        reference = "C_NEAR"
        value = "100 nF"
        footprint_name = "C_0402"
        layer_id = "F.Cu"
        position = [pscustomobject]@{ x_nm = 10000000; y_nm = 17000000 }
      }
      $fixture.board.placement_regions = @($fixture.board.placement_regions) + @([pscustomobject]@{
        id = "PR_SPRINT997"
        kind = "placement"
        area = [pscustomobject]@{
          x_nm = 7000000; y_nm = 16000000; width_nm = 5000000; height_nm = 2000000
        }
      })
    }
    if ($Name.Contains("sprint993")) {
      if (-not $fixture.board.groups) {
        $fixture.board | Add-Member -MemberType NoteProperty -Name groups -Value @() -Force
      }
      $inputComponents = @("JAC1", "JAC2", "D1", "D2", "D3", "D4")
      $groupMembers = @($fixture.board.pads |
        Where-Object { $_.component_id -in $inputComponents -and
                       $_.net_id -in @("AC1", "AC2") } |
        ForEach-Object { [string]$_.id })
      if ($groupMembers.Count -lt 4) {
        throw "Sprint 993 functional-block fixture lacks its serialized AC-input board pads."
      }
      $fixture.board.groups += [pscustomobject]@{
        id = "GROUP_SPRINT993_AC_INPUT"
        name = "AC input stage"
        members = $groupMembers
      }
    }
    [IO.File]::WriteAllText($isolatedProjectPath,
      (ConvertTo-Json -InputObject $fixture -Depth 64), [Text.UTF8Encoding]::new($false))
    if ($Name.Contains("sprint997")) {
      $fixtureCheck = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
      if (@($fixtureCheck.board.footprints | Where-Object { $_.reference -in @("JAC1", "C_NEAR") }).Count -ne 2 -or
          @($fixtureCheck.board.placement_regions | Where-Object { $_.id -eq "PR_SPRINT997" }).Count -ne 1) {
        throw "Sprint 997 disposable board fixture did not serialize its proximity and placement-region records."
      }
    }
  } elseif ($Name.StartsWith("sprint985-project-reference-graph") -or
            $Name.StartsWith("sprint986-project-spatial-index")) {
    $fixture = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
    if (-not $fixture.board) { throw "Project graph GUI scenario requires a board." }
    if (-not $fixture.board.tracks) {
      $fixture.board | Add-Member -MemberType NoteProperty -Name tracks -Value @() -Force
    }
    $syntheticTrackId = if ($Name.StartsWith("sprint986-project-spatial-index")) {
      "T_SPRINT986_ZERO"
    } else { "T_SPRINT985_ZERO" }
    $fixture.board.tracks += [pscustomobject]@{
      id = $syntheticTrackId
      net_id = ""
      layer_id = "F.Cu"
      start = [pscustomobject]@{ x_nm = 12000000; y_nm = 12000000 }
      end = [pscustomobject]@{ x_nm = 12000000; y_nm = 12000000 }
      width_nm = 250000
      source_route_request_id = ""
    }
    [IO.File]::WriteAllText($isolatedProjectPath,
      (ConvertTo-Json -InputObject $fixture -Depth 64), [Text.UTF8Encoding]::new($false))
  }
  $ProjectPath = $isolatedProjectPath
}
if ($Name.StartsWith("sprint998-functional-block-net-context")) {
  $fixture = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
  if (-not $fixture.board.pads) {
    throw "Functional-block net fixture requires a serialized board pad."
  }
  $netPad = @($fixture.board.pads | Where-Object {
    $_.id -and $_.net_id
  } | Select-Object -First 1)
  if ($netPad.Count -ne 1) {
    throw "Functional-block net fixture requires a pad with a native net ID."
  }
  if (-not $fixture.board.groups) {
    $fixture.board | Add-Member -MemberType NoteProperty -Name groups -Value @() -Force
  }
  $fixture.board.groups += [pscustomobject]@{
    id = "GROUP_SPRINT998_RETURN"
    name = "Return path"
    members = @([string]$netPad[0].id)
  }
  [IO.File]::WriteAllText($isolatedProjectPath,
    (ConvertTo-Json -InputObject $fixture -Depth 64), [Text.UTF8Encoding]::new($false))
  $fixtureCheck = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
  $fixtureGroup = @($fixtureCheck.board.groups | Where-Object {
    $_.id -eq "GROUP_SPRINT998_RETURN" -and $_.name -eq "Return path"
  })
  $fixtureMember = if ($fixtureGroup.Count -eq 1) {
    [string]$fixtureGroup[0].members[0]
  } else { "" }
  $fixtureMemberPad = @($fixtureCheck.board.pads | Where-Object {
    $_.id -eq $fixtureMember -and $_.net_id
  })
  if ($fixtureGroup.Count -ne 1 -or $fixtureMemberPad.Count -ne 1) {
    throw "Sprint 998 disposable fixture failed its serialized group-to-native-net round trip."
  }
}
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
if ($Name.StartsWith("sprint991-semantic-memory")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-sprint991-semantic-" + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "sprint991-semantic-memory-ui-thread"
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
if ($Name.StartsWith("sprint1001-memory-kind") -or $Name.StartsWith("sprint1003-memory-importance")) {
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ("ccad-$Name-" + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_MEMORY_PATH = Join-Path $isolatedMemoryProfile "agent_memory.json"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = "$Name-ui-thread"
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
    $Name.StartsWith("sprint981-schematic-project-graph") -or
    $Name.StartsWith("sprint982-multilayer-project-context") -or
    $Name.StartsWith("sprint983-project-index-typed-geometry") -or
    $Name.StartsWith("sprint985-project-reference-graph") -or
    $Name.StartsWith("sprint984-board-net-retrieval") -or
    $Name.StartsWith("sprint986-project-spatial-index") -or
    $Name.StartsWith("sprint987-schematic-metadata") -or
    $Name.StartsWith("sprint998-functional-block-net-context")) {
  $profilePrefix = if ($Name.StartsWith("sprint986-project-spatial-index")) {
    "ccad-sprint986-project-spatial-"
  } elseif ($Name.StartsWith("sprint987-schematic-metadata")) {
    "ccad-sprint987-schematic-metadata-"
  } elseif ($Name.StartsWith("sprint985-project-reference-graph")) {
    "ccad-sprint985-project-graph-"
  } elseif ($Name.StartsWith("sprint984-board-net-retrieval")) {
    "ccad-sprint984-board-net-"
  } elseif ($Name.StartsWith("sprint982-multilayer-project-context")) {
    "ccad-sprint982-multilayer-"
  } elseif ($Name.StartsWith("sprint983-project-index-typed-geometry")) {
    "ccad-sprint983-typed-geometry-"
  } elseif ($Name.StartsWith("sprint981-schematic-project-graph")) {
    "ccad-sprint981-project-graph-"
  } elseif ($Name.StartsWith("sprint998-functional-block-net-context")) {
    "ccad-sprint998-block-net-"
  } else { "ccad-sprint980-project-" }
  $isolatedMemoryProfile = Join-Path ([IO.Path]::GetTempPath()) ($profilePrefix + [Guid]::NewGuid().ToString("N"))
  $configDir = Join-Path $isolatedMemoryProfile "CCad"
  New-Item -ItemType Directory -Path $configDir -Force | Out-Null
  $env:APPDATA = $isolatedMemoryProfile
  $env:CCAD_AGENT_CONVERSATION_DB = Join-Path $isolatedMemoryProfile "agent_conversations.sqlite3"
  $env:CCAD_AGENT_CHECKPOINT_DB = Join-Path $isolatedMemoryProfile "agent_checkpoints.sqlite"
  $env:CCAD_AGENT_THREAD_ID = if ($Name.StartsWith("sprint987-schematic-metadata")) {
    if ($Name.Contains("sprint997")) { "sprint997-geometry-relations-ui-thread" }
    elseif ($Name.Contains("sprint993")) { "sprint993-functional-block-ui-thread" }
    else { "sprint987-schematic-metadata-ui-thread" }
  } elseif ($Name.StartsWith("sprint998-functional-block-net-context")) {
    "sprint998-functional-block-net-context-ui-thread"
  } elseif ($Name.StartsWith("sprint986-project-spatial-index")) {
    "sprint986-project-spatial-index-ui-thread"
  } elseif ($Name.StartsWith("sprint984-board-net-retrieval")) {
    "sprint984-board-net-retrieval-ui-thread"
  } elseif ($Name.StartsWith("sprint982-multilayer-project-context")) {
    "sprint982-multilayer-project-context-ui-thread"
  } elseif ($Name.StartsWith("sprint985-project-reference-graph")) {
    "sprint985-project-reference-graph-ui-thread"
  } elseif ($Name.StartsWith("sprint983-project-index-typed-geometry")) {
    "sprint983-project-index-typed-geometry-ui-thread"
  } elseif ($Name.StartsWith("sprint981-schematic-project-graph")) {
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
  if ($Name.Contains("sprint997")) {
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    $geometryProof = @($reportData.entries | Where-Object {
      $_.pcb_geometry_relationships_visible -eq $true
    } | Select-Object -First 1)
    $turnProof = @($reportData.entries | Where-Object {
      $_.conversation_turn_visible -eq $true
    } | Select-Object -First 1)
    if ($geometryProof.Count -ne 1 -or $turnProof.Count -ne 1) {
      throw "Mapped Agent context did not prove nearby-footprint and placement-region counts. Report: $reportPath"
    }
    if (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet) {
      throw "Provider request occurred during the isolated geometry-context validation."
    }
  }
  if ($Name.StartsWith("sprint982-multilayer-project-context")) {
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    $viaProof = @($reportData.entries | Where-Object {
      $_.multilayer_via_created -eq $true
    } | Select-Object -First 1)
    $layerProof = @($reportData.entries | Where-Object {
      $_.project_layers_visible -eq $true
    } | Select-Object -First 1)
    if ($viaProof.Count -ne 1 -or $layerProof.Count -ne 1) {
      throw "UI-map flow did not prove via placement and included layer context. Report: $reportPath"
    }
    $savedProject = Get-Content -Raw -LiteralPath $isolatedProjectPath | ConvertFrom-Json
    $savedVias = @($savedProject.board.vias | Where-Object {
      $_.id -eq $viaProof[0].via_id -and
      $_.start_layer_id -eq "F.Cu" -and $_.end_layer_id -eq "B.Cu"
    })
    if ($savedVias.Count -ne 1) {
      throw "Disposable project did not persist the mapped via's typed copper layer span."
    }
  }
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
if ($Name.StartsWith("sprint991-semantic-memory")) {
  $configFile = Join-Path $isolatedMemoryProfile "CCad\agent_config.json"
  $config = Get-Content -Raw -LiteralPath $configFile | ConvertFrom-Json
  if (-not $config.memory.semantic.enabled -or
      $config.memory.semantic.backend -ne "ollama_local" -or
      $config.memory.semantic.base_url -ne "http://127.0.0.1:11434" -or
      $config.memory.semantic.model -ne "embeddinggemma") {
    throw "Semantic retrieval preferences were not persisted exactly in the isolated profile."
  }
  $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
  $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
  $mapped = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.click" -and $_.result.result.performed -eq $true
  })
  $typed = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.type_text" -and $_.result.result.performed -eq $true
  })
  if ($mapped.Count -lt 5 -or $typed.Count -lt 2) {
    throw "Semantic settings GUI flow did not perform the expected mapped edits."
  }
  $runtimeStatus = @($reportData.entries | Where-Object {
    $_.semantic_runtime_status_found -eq $true -and
    $_.target.label -match 'Semantic retrieval: (ready|service_unavailable|model_not_installed)'
  })
  if ($runtimeStatus.Count -ne 1) {
    throw "Semantic settings did not display a truthful ready/unavailable installed-model status."
  }
  $checkpoints = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
  if ($checkpoints.Count -ne 4) {
    throw "Semantic settings validation should retain four distinct UI checkpoints; found $($checkpoints.Count)."
  }
  if (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet) {
    throw "Provider request occurred during semantic settings GUI validation."
  }
}
if ($Name.StartsWith("sprint1001-memory-kind")) {
  $memoryFile = $env:CCAD_AGENT_MEMORY_PATH
  if (-not (Test-Path -LiteralPath $memoryFile)) {
    throw "Mapped memory-kind flow did not create its isolated durable store."
  }
  $records = @(Get-Content -Raw -LiteralPath $memoryFile | ConvertFrom-Json)
  $saved = @($records | Where-Object {
    $_.tier -eq "ltm" -and $_.kind -eq "preference" -and
    $_.title -eq "Memory kind proof" -and $_.scope -eq "conversation" -and
    $_.content -eq "Keep ground return routing short in future edits"
  })
  if ($saved.Count -ne 1) {
    throw "Mapped memory-kind flow did not persist exactly one typed preference record."
  }
  $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
  $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
  $performed = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.click" -and $_.result.result.performed -eq $true
  })
  $typed = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.type_text" -and $_.result.result.performed -eq $true
  })
  if ($performed.Count -lt 7 -or $typed.Count -lt 3 -or
      -not ($reportData.entries | Where-Object { $_.memory_kind_saved_visible -eq $true })) {
    throw "Mapped memory-kind flow did not prove mapped editing, save, and visible result."
  }
  $checkpoints = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
  if ($checkpoints.Count -ne 15) {
    throw "Memory-kind proof must retain one screenshot per mapped click/text entry (15 total); found $($checkpoints.Count)."
  }
  if (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet) {
    throw "Provider request occurred during the isolated memory-kind GUI validation."
  }
}
if ($Name.StartsWith("sprint1003-memory-importance")) {
  $memoryFile = $env:CCAD_AGENT_MEMORY_PATH
  if (-not (Test-Path -LiteralPath $memoryFile)) {
    throw "Mapped memory-importance flow did not create its isolated durable store."
  }
  $records = @(Get-Content -Raw -LiteralPath $memoryFile | ConvertFrom-Json)
  $saved = @($records | Where-Object {
    $_.tier -eq "ltm" -and $_.kind -eq "fact" -and $_.importance -eq 5 -and
    $_.title -eq "Memory importance proof" -and $_.scope -eq "conversation" -and
    $_.content -eq "Keep ground return routing short in future edits"
  })
  if ($saved.Count -ne 1) {
    throw "Mapped memory-importance flow did not persist exactly one priority-5 preference."
  }
  $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
  $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
  $performed = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.click" -and $_.result.result.performed -eq $true
  })
  $typed = @($reportData.entries | Where-Object {
    $_.interaction -eq "ui.type_text" -and $_.result.result.performed -eq $true
  })
  if ($performed.Count -lt 8 -or $typed.Count -lt 3 -or
      -not ($reportData.entries | Where-Object { $_.memory_importance_saved_visible -eq $true })) {
    throw "Mapped memory-importance flow did not prove selecting, saving, and displaying priority."
  }
  $checkpoints = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
  if ($checkpoints.Count -ne 16) {
    throw "Memory-importance proof requires 16 scoped interaction screenshots; found $($checkpoints.Count)."
  }
  if (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet) {
    throw "Provider request occurred during isolated memory-importance GUI validation."
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
      $Name.StartsWith("sprint981-schematic-project-graph") -or
      $Name.StartsWith("sprint982-multilayer-project-context") -or
      $Name.StartsWith("sprint983-project-index-typed-geometry") -or
      $Name.StartsWith("sprint984-board-net-retrieval") -or
      $Name.StartsWith("sprint985-project-reference-graph") -or
      $Name.StartsWith("sprint986-project-spatial-index") -or
      $Name.StartsWith("sprint987-schematic-metadata")) {
    $reportPath = Join-Path $ScreenshotDir "$Name-target-sequence.json"
    $reportData = Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    $requiredReportFields = if ($Name.Contains("sprint992")) {
      @("conversation_turn_visible", "schematic_metadata_serialized",
        "declared_pin_serialized", "schematic_metadata_retrieved",
        "schematic_pin_retrieval_visible")
    } elseif ($Name.Contains("sprint993")) {
      @("conversation_turn_visible", "schematic_metadata_serialized",
        "schematic_metadata_retrieved", "functional_block_visible")
    } elseif ($Name.Contains("sprint997")) {
      @("conversation_turn_visible", "pcb_geometry_relationships_visible")
    } elseif ($Name.StartsWith("sprint998-functional-block-net-context")) {
      @("conversation_turn_visible", "functional_block_fixture_loaded",
        "functional_block_net_retrieval_visible")
    } elseif ($Name.StartsWith("sprint987-schematic-metadata")) {
      @("conversation_turn_visible", "schematic_metadata_serialized",
        "schematic_metadata_retrieved")
    } elseif ($Name.StartsWith("sprint985-project-reference-graph") -or
              $Name.StartsWith("sprint986-project-spatial-index")) {
      @("conversation_turn_visible")
    } else { @("conversation_turn_visible", "canonical_transcript_retained_after_clear") }
    foreach ($field in $requiredReportFields) {
      if (-not ($reportData.entries | Where-Object { $_.$field -eq $true })) {
        throw "Mapped conversation scenario did not verify '$field'. Report: $reportPath"
      }
    }
    if (-not $Name.StartsWith("sprint985-project-reference-graph") -and
        -not $Name.StartsWith("sprint986-project-spatial-index") -and
        -not (Test-Path -LiteralPath $env:CCAD_AGENT_CONVERSATION_DB)) {
      throw "GUI run did not create its isolated durable conversation database."
    }
    if ($Name.StartsWith("sprint986-project-spatial-index")) {
      $requiredScreenshots = @("before", "diagnostics-ready", "turn-persisted")
      $actualScreenshots = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
      if ($actualScreenshots.Count -ne $requiredScreenshots.Count) {
        throw "Spatial project GUI validation should retain only three distinct checkpoints; found $($actualScreenshots.Count)."
      }
    } elseif ($Name.Contains("sprint992") -or $Name.Contains("sprint993") -or
              $Name.StartsWith("sprint998-functional-block-net-context")) {
      $requiredScreenshots = @("before", "turn-persisted",
                               "context-settings-dialog", "restored-final")
      $actualScreenshots = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
      if ($actualScreenshots.Count -ne $requiredScreenshots.Count) {
        throw "Schematic pin retrieval GUI validation should retain four distinct checkpoints; found $($actualScreenshots.Count)."
      }
    } elseif ($Name.Contains("sprint997")) {
      $requiredScreenshots = @("before", "turn-persisted",
                               "context-settings-dialog", "restored-final")
      $actualScreenshots = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
      if ($actualScreenshots.Count -ne $requiredScreenshots.Count) {
        throw "Sprint 997 geometry retrieval GUI validation should retain four distinct checkpoints; found $($actualScreenshots.Count)."
      }
    } elseif ($Name.StartsWith("sprint987-schematic-metadata")) {
      $requiredScreenshots = @("before", "turn-persisted", "projection-cleared")
      $actualScreenshots = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png")
      if ($actualScreenshots.Count -ne $requiredScreenshots.Count) {
        throw "Schematic metadata GUI validation should retain only three distinct checkpoints; found $($actualScreenshots.Count)."
      }
    } elseif ($Name.StartsWith("sprint985-project-reference-graph")) {
      $requiredScreenshots = @("before", "turn-persisted")
      foreach ($state in $requiredScreenshots) {
        if (-not (Test-Path -LiteralPath (Join-Path $ScreenshotDir "$Name-$state.png"))) {
          throw "Project graph GUI validation is missing the '$state' checkpoint."
        }
      }
      $interactionScreenshots = @(Get-ChildItem -LiteralPath $ScreenshotDir -Filter "$Name-*.png" |
        Where-Object { $_.Name -notmatch '-(before|turn-persisted)\.png$' })
      if ($interactionScreenshots.Count -lt 7) {
        throw "Project graph GUI validation requires a screenshot for each of seven mapped actions."
      }
    }
    $agentPython = Join-Path $Root "src\ccad_agent\venv\Scripts\python.exe"
    if (-not (Test-Path -LiteralPath $agentPython)) {
      throw "The configured Agent Python runtime is required to inspect the conversation database."
    }
    $databaseVerifier = Join-Path $Root "scripts\verify_conversation_ui_state.py"
    $expectedThreadId = if ($Name.StartsWith("sprint987-schematic-metadata")) {
      if ($Name.Contains("sprint997")) { "sprint997-geometry-relations-ui-thread" }
      elseif ($Name.Contains("sprint993")) { "sprint993-functional-block-ui-thread" }
      else { "sprint987-schematic-metadata-ui-thread" }
    } elseif ($Name.StartsWith("sprint986-project-spatial-index")) {
      "sprint986-project-spatial-index-ui-thread"
    } elseif ($Name.StartsWith("sprint985-project-reference-graph")) {
      "sprint985-project-reference-graph-ui-thread"
    } elseif ($Name.StartsWith("sprint984-board-net-retrieval")) {
      "sprint984-board-net-retrieval-ui-thread"
    } elseif ($Name.StartsWith("sprint982-multilayer-project-context")) {
      "sprint982-multilayer-project-context-ui-thread"
    } elseif ($Name.StartsWith("sprint983-project-index-typed-geometry")) {
      "sprint983-project-index-typed-geometry-ui-thread"
    } elseif ($Name.StartsWith("sprint981-schematic-project-graph")) {
      "sprint981-schematic-project-graph-ui-thread"
    } elseif ($Name.StartsWith("sprint980-project-retrieval")) {
      "sprint980-project-retrieval-ui-thread"
    } elseif ($Name.StartsWith("sprint977-context")) {
      "sprint977-context-ui-thread"
    } else {
      "sprint976-conversation-ui-thread"
    }
    if ($Name.StartsWith("sprint985-project-reference-graph") -or
        $Name.StartsWith("sprint986-project-spatial-index")) {
      $databaseState = $null
    } else {
      $databaseStateJson = & $agentPython $databaseVerifier $env:CCAD_AGENT_CONVERSATION_DB $expectedThreadId
      if ($LASTEXITCODE -ne 0) { throw "Could not inspect the isolated conversation database." }
      $databaseState = $databaseStateJson | ConvertFrom-Json
    }
    if (-not $Name.StartsWith("sprint985-project-reference-graph") -and
        -not $Name.StartsWith("sprint986-project-spatial-index") -and
        ($databaseState.messages -ne 2 -or $databaseState.users -ne 1 -or
        $databaseState.assistants -ne 1 -or $databaseState.turn_records -ne 1 -or
        $databaseState.projection -ne "[]")) {
      throw "Unexpected persisted transcript/projection state: $databaseStateJson"
    }
    if ($Name.StartsWith("sprint977-context") -and
        -not ($reportData.entries | Where-Object { $_.context_memory_attached -eq $true })) {
      throw "Mapped turn did not visibly prove inclusion of its enabled scoped memory entry."
    }
    if (($Name.StartsWith("sprint980-project-retrieval") -or
         $Name.StartsWith("sprint981-schematic-project-graph") -or
         $Name.StartsWith("sprint982-multilayer-project-context") -or
         $Name.StartsWith("sprint983-project-index-typed-geometry") -or
         $Name.StartsWith("sprint984-board-net-retrieval") -or
         $Name.StartsWith("sprint985-project-reference-graph") -or
         $Name.StartsWith("sprint986-project-spatial-index") -or
         $Name.StartsWith("sprint987-schematic-metadata")) -and
        -not ($reportData.entries | Where-Object { $_.project_retrieval_visible -eq $true })) {
      throw "Mapped turn did not visibly prove a non-empty typed-project retrieval result."
    }
    if ($databaseState) {
      $databaseState | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ScreenshotDir "$Name-database-verification.json")
    }
    if ($stdoutLog -and (Select-String -LiteralPath $stdoutLog -Pattern 'provider_request_sent":true' -Quiet)) {
      throw "Provider request occurred during conversation UI validation."
    }
    if ($Name.StartsWith("sprint982-multilayer-project-context") -and
        -not ($reportData.entries | Where-Object { $_.project_layers_visible -eq $true })) {
      throw "Mapped turn did not visibly include the via's F.Cu/B.Cu layer identities."
    }
    if ($Name.StartsWith("sprint983-project-index-typed-geometry") -and
        -not ($reportData.entries | Where-Object { $_.multiple_project_layers_visible -eq $true })) {
      throw "Mapped turn did not prove multiple exact PCB layer identities reached bounded context."
    }
    if ($Name.StartsWith("sprint984-board-net-retrieval") -and
        -not ($reportData.entries | Where-Object { $_.board_net_count_visible -eq $true })) {
      throw "Mapped turn did not show native PCB net retrieval in safe context metadata."
    }
    if ($Name.StartsWith("sprint985-project-reference-graph") -and
        (-not ($reportData.entries | Where-Object { $_.project_diagnostic_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.project_diagnostic_count_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.live_diagnostic_object_link_verified -eq $true }))) {
      throw "Mapped run did not verify live DRC/ERC object identity and per-turn context count."
    }
    if ($Name.StartsWith("sprint986-project-spatial-index") -and
        (-not ($reportData.entries | Where-Object { $_.project_diagnostic_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.project_diagnostic_count_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.live_diagnostic_object_link_verified -eq $true }))) {
      throw "Spatial context turn did not carry the live object-linked diagnostic into bounded Agent context."
    }
    if ($Name.StartsWith("sprint981-schematic-project-graph") -and
        (-not ($reportData.entries | Where-Object { $_.schematic_pin_retrieval_visible -eq $true }) -or
         -not ($reportData.entries | Where-Object { $_.schematic_symbol_retrieval_visible -eq $true }))) {
      throw "Mapped turn did not visibly include retrieved schematic net pins and related symbols."
    }
    $requiredScreenshots = if ($Name.Contains("sprint992") -or $Name.Contains("sprint993") -or
                               $Name.StartsWith("sprint998-functional-block-net-context")) {
      @("before", "turn-persisted", "context-settings-dialog", "restored-final")
    } elseif ($Name.Contains("sprint997")) {
      @("before", "turn-persisted", "context-settings-dialog", "restored-final")
    } elseif ($Name.StartsWith("sprint986-project-spatial-index")) {
      @("before", "diagnostics-ready", "turn-persisted")
    } elseif ($Name.StartsWith("sprint987-schematic-metadata")) {
      @("before", "turn-persisted", "projection-cleared")
    } elseif ($Name.StartsWith("sprint985-project-reference-graph")) {
      @("before", "turn-persisted")
    } elseif ($Name.StartsWith("sprint982-multilayer-project-context")) {
      @("before", "via-placed", "turn-persisted", "projection-cleared")
    } elseif ($Name.StartsWith("sprint981-schematic-project-graph")) {
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
  if ($isolatedProjectPath -and (Test-Path -LiteralPath $isolatedProjectPath)) {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $projectFile = [IO.Path]::GetFullPath($isolatedProjectPath)
    if (-not $projectFile.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        -not ([IO.Path]::GetFileName($projectFile) -like "ccad-sprint982-multilayer-*.ccad.json" -or
              [IO.Path]::GetFileName($projectFile) -like "ccad-sprint983-typed-geometry-*.ccad.json" -or
              [IO.Path]::GetFileName($projectFile) -like "ccad-sprint985-project-graph-*.ccad.json" -or
              [IO.Path]::GetFileName($projectFile) -like "ccad-sprint986-project-spatial-*.ccad.json" -or
              [IO.Path]::GetFileName($projectFile) -like "ccad-sprint987-schematic-metadata-*.ccad.json" -or
              [IO.Path]::GetFileName($projectFile) -like "ccad-sprint998-block-net-*.ccad.json")) {
      throw "Refusing to remove a project outside the verified temporary targets."
    }
    Remove-Item -LiteralPath $projectFile -Force
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
