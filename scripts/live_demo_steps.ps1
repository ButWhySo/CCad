$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path ".\").Path
$Ccad = "$Root\build-qt\ccad.exe"
$Project = "$Root\cli_demo.json"

Write-Output "Live demo starting. Setting up initial project..."
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
            x_nm = 0; y_nm = 0
            width_nm = 60000000; height_nm = 50000000
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
& $Ccad pcb add-standard-layers --file $Project
Start-Sleep -Seconds 2

Write-Output "Adding outline..."
& $Ccad pcb set-outline --file $Project --x-mm 0 --y-mm 0 --width-mm 60 --height-mm 50
Start-Sleep -Seconds 1

Write-Output "Adding components..."
& $Ccad pcb add-pad --file $Project --id JAC1.1 --component JAC1 --pin 1 --net AC1 --layers F.Cu --x-mm 5 --y-mm 20 --width-mm 2 --height-mm 2 --type smd --shape rect
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id JAC2.1 --component JAC2 --pin 1 --net AC2 --layers F.Cu --x-mm 5 --y-mm 40 --width-mm 2 --height-mm 2 --type smd --shape rect
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D1.A --component D1 --pin A --net AC1 --layers F.Cu --x-mm 15 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D1.K --component D1 --pin K --net DC_POS --layers F.Cu --x-mm 25 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D2.A --component D2 --pin A --net AC2 --layers F.Cu --x-mm 35 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D2.K --component D2 --pin K --net DC_POS --layers F.Cu --x-mm 25 --y-mm 10 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D3.K --component D3 --pin K --net AC1 --layers F.Cu --x-mm 15 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D3.A --component D3 --pin A --net DC_NEG --layers F.Cu --x-mm 25 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D4.K --component D4 --pin K --net AC2 --layers F.Cu --x-mm 35 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id D4.A --component D4 --pin A --net DC_NEG --layers F.Cu --x-mm 25 --y-mm 30 --width-mm 2 --height-mm 2 --type smd --shape roundrect --roundrect-rratio 0.25
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id C1.1 --component C1 --pin 1 --net DC_POS --layers F.Cu --x-mm 45 --y-mm 15 --width-mm 2 --height-mm 2 --type smd --shape circle
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id C1.2 --component C1 --pin 2 --net DC_NEG --layers F.Cu --x-mm 45 --y-mm 25 --width-mm 2 --height-mm 2 --type smd --shape circle
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id JDC1.1 --component JDC1 --pin 1 --net DC_POS --layers F.Cu --x-mm 55 --y-mm 15 --width-mm 2 --height-mm 2 --type smd --shape rect
Start-Sleep -Milliseconds 200
& $Ccad pcb add-pad --file $Project --id JDC2.1 --component JDC2 --pin 1 --net DC_NEG --layers F.Cu --x-mm 55 --y-mm 25 --width-mm 2 --height-mm 2 --type smd --shape rect
Start-Sleep -Seconds 1

Write-Output "Routing tracks..."
& $Ccad pcb add-track --file $Project --id TAC1.1 --net AC1 --layer F.Cu --start-x-mm 15 --start-y-mm 10 --end-x-mm 15 --end-y-mm 30 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TAC1.2 --net AC1 --layer F.Cu --start-x-mm 5 --start-y-mm 20 --end-x-mm 15 --end-y-mm 20 --width-mm 0.5
Start-Sleep -Milliseconds 200

& $Ccad pcb add-track --file $Project --id TAC2.1 --net AC2 --layer F.Cu --start-x-mm 35 --start-y-mm 10 --end-x-mm 35 --end-y-mm 30 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TAC2.2 --net AC2 --layer F.Cu --start-x-mm 5 --start-y-mm 40 --end-x-mm 35 --end-y-mm 40 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TAC2.3 --net AC2 --layer F.Cu --start-x-mm 35 --start-y-mm 40 --end-x-mm 35 --end-y-mm 30 --width-mm 0.5
Start-Sleep -Milliseconds 200

& $Ccad pcb add-track --file $Project --id TPOS.1 --net DC_POS --layer F.Cu --start-x-mm 25 --start-y-mm 10 --end-x-mm 45 --end-y-mm 10 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TPOS.2 --net DC_POS --layer F.Cu --start-x-mm 45 --start-y-mm 10 --end-x-mm 45 --end-y-mm 15 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TPOS.3 --net DC_POS --layer F.Cu --start-x-mm 45 --start-y-mm 15 --end-x-mm 55 --end-y-mm 15 --width-mm 0.5
Start-Sleep -Milliseconds 200

& $Ccad pcb add-track --file $Project --id TNEG.1 --net DC_NEG --layer F.Cu --start-x-mm 25 --start-y-mm 30 --end-x-mm 45 --end-y-mm 30 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TNEG.2 --net DC_NEG --layer F.Cu --start-x-mm 45 --start-y-mm 30 --end-x-mm 45 --end-y-mm 25 --width-mm 0.5
Start-Sleep -Milliseconds 200
& $Ccad pcb add-track --file $Project --id TNEG.3 --net DC_NEG --layer F.Cu --start-x-mm 45 --start-y-mm 25 --end-x-mm 55 --end-y-mm 25 --width-mm 0.5
Start-Sleep -Milliseconds 200

Write-Output "Live demo sequence complete!"
& $Ccad drc $Project
