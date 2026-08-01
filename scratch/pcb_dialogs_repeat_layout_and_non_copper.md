# KiCad Exploration - Batch 33: Multichannel Base Classes, Repeat Layout, Non-Copper Zones

## Overview
This batch covers the base UI classes for generating multichannel rule areas, the logic and UI for the "Repeat Multichannel Layout" dialog, and the properties dialog for non-copper zones (e.g., keepouts, graphic polygons on silk/mask layers).

## Key Findings

### Multichannel Generate Rule Areas (Base)
- **`dialog_multichannel_generate_rule_areas_base.cpp/.h`**:
  - The WxFormBuilder base classes for the dialog covered in Batch 32.
  - Contains the notebook for Sheets/Component Classes and the checkboxes for replacing existing and grouping items.

### Multichannel Repeat Layout
- **`dialog_multichannel_repeat_layout.cpp/.h` & `dialog_multichannel_repeat_layout_base.cpp/.h`**:
  - Dialog used in the multichannel tool to copy the layout (placement/routing) from a reference rule area to one or more target rule areas.
  - **`TABLE_ENTRY` struct** tracks `m_targetRA`, `m_isOK`, `m_doCopy`, `m_raName`, `m_errMsg`, and `m_mismatchReasons`.
  - UI includes:
    - **Reference rule area label**: Shows the source.
    - **Anchor footprint**: Dropdown to select a specific footprint as an anchor for precise/rotated placement.
    - **Target areas grid**: Lists matching rule areas. Shows "OK" or an error message (with a details icon for mismatches). Clicking the details icon triggers `MULTICHANNEL_TOOL::ShowMismatchDetails()`.
    - **Options**: Copy placement, copy routing, restrict to connected routing, copy other items, group items, include locked components.
  - The dialog passes user configuration back to `RULE_AREAS_DATA`.

### Non-Copper Zones Properties
- **`dialog_non_copper_zones_properties.cpp` & `dialog_non_copper_zones_properties_base.cpp`**:
  - `DIALOG_NON_COPPER_ZONES_EDITOR`: Used to edit properties of non-copper zones (polygons on technical layers like Edge.Cuts, SilkS, Mask) or to configure the conversion of graphical items to a non-copper zone.
  - Takes a `ZONE_SETTINGS*` and an optional `CONVERT_SETTINGS*`.
  - If `aConvertSettings` is provided, a "Conversion Settings" box is prepended to the UI (radio buttons for Centerline vs Bounding Hull, Gap text control, Delete Originals checkbox).
  - Uses `UNIT_BINDER` for minimum width, outline hatch pitch, hatch width, hatch gap, corner radius, and gap.
  - **Layer Selection**: Uses a `wxDataViewListCtrl` (`m_layers`) initialized with `LSET::AllNonCuMask()`.
  - **Zone Fill Mode**: Solid fill or Hatch pattern. If Hatch pattern, options for hatch width, gap, rotation, and smoothing level/amount are enabled.
  - **Outline Style**: Line, Hatched, Fully hatched.
  - Maps UI inputs back into the `ZONE_SETTINGS` struct upon OK.
