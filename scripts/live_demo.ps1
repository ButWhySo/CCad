$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path ".\").Path
$Gui = "$Root\build-qt\ccad_gui.exe"
$Ccad = "$Root\build-qt\ccad.exe"

$Project = "$Root\live_demo.json"

if (Test-Path $Project) {
    Remove-Item $Project
}

Write-Output "Initializing fresh project..."
$ProjectObject = @{
    schema_version = 2
    components = @(
        [ordered]@{ id = "JAC1"; part = "AC input"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
        [ordered]@{ id = "JAC2"; part = "AC input"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
        [ordered]@{ id = "JDC1"; part = "DC positive"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
        [ordered]@{ id = "JDC2"; part = "DC negative"; pins = @([ordered]@{ name = "1"; kind = "passive" }) },
        [ordered]@{ id = "D1"; part = "Diode"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
        [ordered]@{ id = "D2"; part = "Diode"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
        [ordered]@{ id = "D3"; part = "Diode"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
        [ordered]@{ id = "D4"; part = "Diode"; pins = @([ordered]@{ name = "A"; kind = "passive" }, [ordered]@{ name = "K"; kind = "passive" }) },
        [ordered]@{ id = "C1"; part = "Capacitor"; pins = @([ordered]@{ name = "1"; kind = "passive" }, [ordered]@{ name = "2"; kind = "passive" }) }
    )
    nets = @(
        [ordered]@{ id = "AC1"; members = @(
            [ordered]@{ component_id = "JAC1"; pin_name = "1" },
            [ordered]@{ component_id = "D1"; pin_name = "A" },
            [ordered]@{ component_id = "D3"; pin_name = "K" }
        ) },
        [ordered]@{ id = "AC2"; members = @(
            [ordered]@{ component_id = "JAC2"; pin_name = "1" },
            [ordered]@{ component_id = "D2"; pin_name = "A" },
            [ordered]@{ component_id = "D4"; pin_name = "K" }
        ) },
        [ordered]@{ id = "DC_POS"; members = @(
            [ordered]@{ component_id = "JDC1"; pin_name = "1" },
            [ordered]@{ component_id = "D1"; pin_name = "K" },
            [ordered]@{ component_id = "D2"; pin_name = "K" },
            [ordered]@{ component_id = "C1"; pin_name = "1" }
        ) },
        [ordered]@{ id = "DC_NEG"; members = @(
            [ordered]@{ component_id = "JDC2"; pin_name = "1" },
            [ordered]@{ component_id = "D3"; pin_name = "A" },
            [ordered]@{ component_id = "D4"; pin_name = "A" },
            [ordered]@{ component_id = "C1"; pin_name = "2" }
        ) }
    )
    board = @{
        outline = @{
            x_nm = 0
            y_nm = 0
            width_nm = 50000000
            height_nm = 40000000
        }
        layers = @()
        pads = @()
        vias = @()
        tracks = @()
        zones = @()
        keepouts = @()
        placement_regions = @()
        graphics = @()
        texts = @()
        barcodes = @()
        targets = @()
        route_requests = @()
    }
}
$ProjectJson = $ProjectObject | ConvertTo-Json -Depth 32
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Project, $ProjectJson, $Utf8NoBom)

# Load standard layers to make board valid
& $Ccad pcb add-standard-layers --file $Project

# Start GUI
Write-Output "Starting GUI..."
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
$GuiProcess = Start-Process -FilePath $Gui -ArgumentList $Project -PassThru -NoNewWindow
Start-Sleep -Seconds 3

# Wait for GUI
Write-Output "Designing..."

& $Ccad pcb set-outline --file $Project --x-mm 0 --y-mm 0 --width-mm 50 --height-mm 40
Start-Sleep -Seconds 1

& $Ccad pcb add-pad --file $Project --id JAC1.1 --component JAC1 --pin 1 --net AC1 --layers "F.Cu,F.Paste,F.Mask" --x-mm 5 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape rect
& $Ccad pcb add-pad --file $Project --id JAC2.1 --component JAC2 --pin 1 --net AC2 --layers "F.Cu,F.Paste,F.Mask" --x-mm 5 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape rect
Start-Sleep -Seconds 1

& $Ccad pcb add-pad --file $Project --id D1.A --component D1 --pin A --net AC1 --layers "F.Cu,F.Paste,F.Mask" --x-mm 15 --y-mm 10 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25
& $Ccad pcb add-pad --file $Project --id D1.K --component D1 --pin K --net DC_POS --layers "F.Cu,F.Paste,F.Mask" --x-mm 18 --y-mm 10 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25

& $Ccad pcb add-pad --file $Project --id D2.A --component D2 --pin A --net AC2 --layers "F.Cu,F.Paste,F.Mask" --x-mm 15 --y-mm 30 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25
& $Ccad pcb add-pad --file $Project --id D2.K --component D2 --pin K --net DC_POS --layers "F.Cu,F.Paste,F.Mask" --x-mm 18 --y-mm 30 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25

& $Ccad pcb add-pad --file $Project --id D3.A --component D3 --pin A --net DC_NEG --layers "F.Cu,F.Paste,F.Mask" --x-mm 15 --y-mm 15 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25
& $Ccad pcb add-pad --file $Project --id D3.K --component D3 --pin K --net AC1 --layers "F.Cu,F.Paste,F.Mask" --x-mm 18 --y-mm 15 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25

& $Ccad pcb add-pad --file $Project --id D4.A --component D4 --pin A --net DC_NEG --layers "F.Cu,F.Paste,F.Mask" --x-mm 15 --y-mm 25 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25
& $Ccad pcb add-pad --file $Project --id D4.K --component D4 --pin K --net AC2 --layers "F.Cu,F.Paste,F.Mask" --x-mm 18 --y-mm 25 --width-mm 1.5 --height-mm 1.5 --type smd --shape roundrect --roundrect-rratio 0.25

Start-Sleep -Seconds 1
& $Ccad pcb add-pad --file $Project --id C1.1 --component C1 --pin 1 --net DC_POS --layers "*.Cu,*.Mask" --x-mm 25 --y-mm 15 --width-mm 2 --height-mm 2 --type thru_hole --shape circle --drill-mm 1.0
& $Ccad pcb add-pad --file $Project --id C1.2 --component C1 --pin 2 --net DC_NEG --layers "*.Cu,*.Mask" --x-mm 25 --y-mm 25 --width-mm 2 --height-mm 2 --type thru_hole --shape circle --drill-mm 1.0

& $Ccad pcb add-pad --file $Project --id JDC1.1 --component JDC1 --pin 1 --net DC_POS --layers "F.Cu,F.Paste,F.Mask" --x-mm 40 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape rect
& $Ccad pcb add-pad --file $Project --id JDC2.1 --component JDC2 --pin 1 --net DC_NEG --layers "F.Cu,F.Paste,F.Mask" --x-mm 40 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape rect

Start-Sleep -Seconds 1
& $Ccad pcb add-track --file $Project --id TAC1.1 --net AC1 --layer F.Cu --start-x-mm 5 --start-y-mm 10 --end-x-mm 15 --end-y-mm 10 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TAC1.2 --net AC1 --layer F.Cu --start-x-mm 15 --start-y-mm 10 --end-x-mm 18 --end-y-mm 15 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TAC2.1 --net AC2 --layer F.Cu --start-x-mm 5 --start-y-mm 30 --end-x-mm 15 --end-y-mm 30 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TAC2.2 --net AC2 --layer F.Cu --start-x-mm 15 --start-y-mm 30 --end-x-mm 18 --end-y-mm 25 --width-mm 0.5

Start-Sleep -Seconds 1
& $Ccad pcb add-track --file $Project --id TPOS.1 --net DC_POS --layer F.Cu --start-x-mm 18 --start-y-mm 10 --end-x-mm 25 --end-y-mm 15 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TPOS.2 --net DC_POS --layer F.Cu --start-x-mm 18 --start-y-mm 30 --end-x-mm 25 --end-y-mm 15 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TPOS.3 --net DC_POS --layer F.Cu --start-x-mm 25 --start-y-mm 15 --end-x-mm 40 --end-y-mm 10 --width-mm 0.5

Start-Sleep -Seconds 1
& $Ccad pcb add-track --file $Project --id TNEG.1 --net DC_NEG --layer F.Cu --start-x-mm 15 --start-y-mm 15 --end-x-mm 25 --end-y-mm 25 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TNEG.2 --net DC_NEG --layer F.Cu --start-x-mm 15 --start-y-mm 25 --end-x-mm 25 --end-y-mm 25 --width-mm 0.5
& $Ccad pcb add-track --file $Project --id TNEG.3 --net DC_NEG --layer F.Cu --start-x-mm 25 --start-y-mm 25 --end-x-mm 40 --end-y-mm 30 --width-mm 0.5

Start-Sleep -Seconds 1
& $Ccad pcb add-text --file $Project --id BT_TITLE --layer F.SilkS --text "FULL BRIDGE RECTIFIER" --x-mm 10 --y-mm 5 --size-x-mm 1.5 --size-y-mm 1.5 --rotation-deg 0
& $Ccad pcb add-barcode --file $Project --id BC1 --layer F.SilkS --text "CCad Live Demo" --kind QRCode --x-mm 40 --y-mm 20 --size-x-mm 8 --size-y-mm 8
& $Ccad pcb add-target --file $Project --id TGT1 --layer F.SilkS --shape Plus --x-mm 5 --y-mm 5 --size-mm 3 --line-width-mm 0.25
& $Ccad pcb add-target --file $Project --id TGT2 --layer F.SilkS --shape X --x-mm 45 --y-mm 5 --size-mm 3 --line-width-mm 0.25

Start-Sleep -Seconds 2
& $Ccad pcb add-zone --file $Project --id Z_GND --name "Ground Pour" --net DC_NEG --layers B.Cu --x-mm 2 --y-mm 2 --width-mm 46 --height-mm 36 --priority 1 --clearance-mm 0.25 --min-thickness-mm 0.25 --pad-connection thermal
Start-Sleep -Seconds 2

Write-Output "Stopping GUI..."
Stop-Process -Id $GuiProcess.Id -Force
Write-Output "Done!"
