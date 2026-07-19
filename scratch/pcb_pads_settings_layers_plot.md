# KiCad Source Walk: Batch 2 (Pads, Settings, Layers, Origin & Plot)

## Overview
This batch continues the deep ingestion of the KiCad `pcbnew` source tree, focusing on padstack definitions, PCB settings, layer selection UI, origin transformations, and plot generation.

- **Files**: 
  - `pcbnew/padstack.h`
  - `pcbnew/pcbnew_settings.cpp`
  - `pcbnew/pcb_layer_box_selector.cpp`, `pcbnew/pcb_layer_box_selector.h`
  - `pcbnew/pcb_layer_presentation.h`
  - `pcbnew/pcb_origin_transforms.cpp`, `pcbnew/pcb_origin_transforms.h`
  - `pcbnew/pcb_plotter.cpp`

## 1. Padstack Definition (`padstack.h`)
- **Purpose**: Defines the `PADSTACK` class, representing a multi-layer pad in the IPC sense. It encapsulates geometry for copper, soldermask, paste, drill features, thermal reliefs, and back-drilling.
- **Mechanism**: Groups layer-specific features into `COPPER_LAYER_PROPS`, `MASK_LAYER_PROPS`, `DRILL_PROPS`, and `POST_MACHINING_PROPS`. Uses an enum `MODE` to distinguish between NORMAL (same on all layers), FRONT_INNER_BACK, and CUSTOM geometry.
- **CCad Analogue**: CCad has a strong typed native equivalent for multi-layer pad geometry generation. However, CCad’s approach to multi-layer routing rules might be more strictly integrated into the layout engine rather than serialized as independent shape sets per pad.

## 2. PCB Settings (`pcbnew_settings.cpp`)
- **Purpose**: Manages application-wide settings for the PCB editor, including AUI layout, footprint chooser sizes, editing behaviors (magnetic pads/tracks), and display options.
- **Mechanism**: Inherits from `APP_SETTINGS_BASE`. Registers parameters to JSON paths (e.g. `aui.show_layer_manager`, `editing.track_drag_action`). Includes extensive legacy migration code from `.ini`/wxConfig files.
- **CCad Analogue**: CCad relies on an LLM-native JSON configuration model without legacy wxConfig migration. CCad GUI settings are minimal, as the design truth lies in the kernel.

## 3. Layer Box Selector (`pcb_layer_box_selector.h`, `.cpp`) & Presentation (`pcb_layer_presentation.h`)
- **Purpose**: Provides a wxBitmapComboBox for selecting active PCB layers, complete with color swatches indicating layer colors and "(not activated)" tags for disabled layers.
- **Mechanism**: `PCB_LAYER_BOX_SELECTOR` inherits `LAYER_BOX_SELECTOR` and interacts with `PCB_LAYER_PRESENTATION` to fetch layer colors and hotkeys for the dropdown list.
- **CCad Analogue**: CCad’s GUI layer management is a Qt-based dock widget querying `ccad_core` layer IDs. We will not use wxWidgets controls. Layer presentation will map directly to the Qt scene graph colors.

## 4. Origin Transforms (`pcb_origin_transforms.h`, `.cpp`)
- **Purpose**: Translates coordinates between the internal system origin and the user-defined display origin. Also handles axis inversion (e.g., inverting the Y axis).
- **Mechanism**: Uses simple algebraic translation based on `m_pcbBaseFrame.GetUserOrigin()` and boolean inversion flags `invertXAxis()`/`invertYAxis()`.
- **CCad Analogue**: CCad natively uses a strict internal coordinate system (usually standard math Cartesian where +Y is up, or standard screen where +Y is down). View transformations are handled natively by the Qt `QGraphicsView` transform matrix rather than manual point-by-point algebraic inversion scattered through the code.

## 5. PCB Plotter (`pcb_plotter.cpp`)
- **Purpose**: Generates manufacturing outputs (Gerber, PDF, DXF, SVG, PostScript, PNG) by iterating over board layers and drawing them to a plot engine.
- **Mechanism**: Validates layer selections, handles bounding boxes for SVG scaling, sets up gerber job file writers, and loops through the selected layers invoking `PlotBoardLayers`. Retrieves plot parameters from `PCB_PLOT_PARAMS`.
- **CCad Analogue**: CCad's export mechanisms will leverage modern geometry libraries directly exporting standard formats (e.g., standard Gerber X2). The plot engine abstraction will be handled completely within the `ccad_core` kernel without being coupled to `PCB_BASE_FRAME` or wxWidgets.
