# Project Progress Counter

This file is the canonical phase/sprint counter for local CCad agent work.

## Current Position

- Phase: 9 / 9
- Phase name: Deterministic KiCad Parity Execution
- Sprint: 351
- Branch: `main`
- Phase 9 sprint budget: Sprints 226 through 254 for deterministic KiCad PCB editor source-walk parity, schematic editor parity, external EDA formats, Gerber viewer, 3D viewer, multi-document projects, library losslessness, live GUI-map performance, prompt/tool-guide assets, observability, and autorouter integration. Sprint 226 root file walk is completely audited.

Phase 6 focused on Agent protocol: JSON-RPC/MCP over the transaction bus, permission gates, benchmark harness. Phase 7 closed the first native Agent pane, GUI parity, and visual-validation backlog budget through Sprint 205. Phase 8 covered the bounded runtime and EDA evidence expansion through Sprint 225. Phase 9 is the bounded KiCad parity execution phase.

**Current State**: 
- [x] Phase 4: LangGraph Python Bridge Architecture Refactoring (Sprint 211)
- [x] Phase 5: Python IPC Process Wrapper Implementation (Sprint 212)
- [x] Phase 6: True Tool Bridging with ToolBroker and ContextBuilder
- [x] Phase 7: Live LangGraph Integration with Context
- **Sprint 355**: Selection Cross-Probing (`selection_cross_probing`).
  - **Goal**: Sync selections between schematic and PCB canvases.
  - **Status**: Completed. Merged into `ccad_gui`.
- **Sprint 354**: Drag-to-Move Tool (`drag_to_move`).
  - **Goal**: Enable drag-to-move for footprints after placement.
  - **Status**: Completed. Merged into `ccad_gui`.
- **Sprint 353**: GUI Docking Framework (`gui_docking`).
  - **Goal**: Convert fixed side panels to QDockWidgets and hide diagnostics by default.
  - **Status**: Completed. Merged into `ccad_gui`.
- **Sprint 352**: KiCad Source Walk & Backlog Generation (`backlog_generation`).
  - **Goal**: Scrape KiCad source to identify missing GUI functionality.
  - **Status**: Completed. Updated `backlog.md`.
- **Sprint 351**: Plugin API Registry (`plugin_api_registry`).
  - **Goal**: Implement central C++ registry for third-party extensions.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 350**: Event-Driven Ratnest Update (`event_driven_ratnest`).
  - **Goal**: Implement event hooks to trigger ratnest recomputes upon physical layout changes.
  - **Status**: Completed. Merged into `ccad_core`. Phase 22 complete.
- **Sprint 349**: Nearest Neighbor Connectivity (`nearest_neighbor_connectivity`).
  - **Goal**: Implement MST logic for optimal shortest-path flying lead calculation.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 348**: Dynamic Ratnest Graph (`dynamic_ratnest_graph`).
  - **Goal**: Implement physical unrouted node tracking structures for ratnest calculation.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 347**: Multi-Channel Cross-Probing (`multi_channel_probe`).
  - **Goal**: Implement physical-to-schematic bidirectional selection mapping for hierarchical instances.
  - **Status**: Completed. Merged into `ccad_core`. Phase 21 complete.
- **Sprint 346**: Board Room Replication (`board_room_replicator`).
  - **Goal**: Implement mathematical footprint stamping/replication across identical hierarchical rooms.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 345**: Hierarchical Sheet Parsing (`hierarchical_sheet_parser`).
  - **Goal**: Implement UUID-based parsing for footprint clustering into logical layout rooms.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 344**: Net-Tie Constraint Checker (`net_tie_drc`).
  - **Goal**: Implement DRC suppression for intentionally shorted nets across registered footprint areas.
  - **Status**: Completed. Merged into `ccad_core`. Phase 20 complete.
- **Sprint 343**: Custom DRC Rules Engine (`custom_drc_rules`).
  - **Goal**: Implement parsing logic for text-based rule constraints.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 342**: Diff Pair DRC (`diff_pair_drc`).
  - **Goal**: Implement DRC evaluation for differential pair limits (skew, length, gap).
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 341**: Differential Pair Tuning (`diff_pair_tuning`).
  - **Goal**: Implement phase skew tracking and coupled serpentine meandering.
  - **Status**: Completed. Merged into `ccad_core`. Phase 19 complete.
- **Sprint 340**: Track Length Tuning (`track_length_tuning`).
  - **Goal**: Implement serpentine meandering logic to achieve target track lengths.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 339**: Teardrop Generator (`teardrop_generator`).
  - **Goal**: Implement teardrop calculation tracking and stub logic for pads/vias.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 338**: 3D Camera Controls (`camera_3d_controls`).
  - **Goal**: Implement matrix transformations and isometric presets for external 3D viewer overlays.
  - **Status**: Completed. Merged into `ccad_core`. Phase 18 complete.
- **Sprint 337**: 3D Scene Graph Bridge (`scene_3d_graph_bridge`).
  - **Goal**: Implement translation of 2D geometry and models into a 3D bounding volume scene graph.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 336**: 3D Model Registry (`model_3d_registry`).
  - **Goal**: Implement associations between physical footprints and external 3D step/wrl models.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 335**: Position File Exporter (`position_file_exporter`).
  - **Goal**: Implement scaffolding to traverse footprints and emit Pick and Place CPL files.
  - **Status**: Completed. Merged into `ccad_core`. Phase 17 complete.
- **Sprint 334**: Drill File Exporter (`drill_file_exporter`).
  - **Goal**: Implement scaffolding to traverse vias/pads and emit Excellon drill format.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 333**: Gerber Plotter Bridge (`gerber_plotter_bridge`).
  - **Goal**: Implement backend traversal scaffolding for emitting Gerber RS-274X plot commands.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 332**: Zone Settings (`zone_settings`).
  - **Goal**: Implement schema for default copper pour constraints and thermal reliefs.
  - **Status**: Completed. Merged into `ccad_core`. Phase 16 complete.
- **Sprint 331**: Board Stackup Manager (`board_stackup_manager`).
  - **Goal**: Implement physical Z-axis definition of the PCB layers and materials.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 330**: Board Design Settings (`board_design_settings`).
  - **Goal**: Implement schema for global project routing defaults (trace widths, via sizes).
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 329**: Undo/Redo Engine (`undo_redo`).
  - **Goal**: Implement the transactional stack for reverting board changes.
  - **Status**: Completed. Merged into `ccad_core`. Phase 15 complete.
- **Sprint 328**: Net Chain Bridging (`net_chain_bridging`).
  - **Goal**: Add logic to assign logical net codes to detached contiguous track segments.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 327**: Tracks Cleaner (`tracks_cleaner`).
  - **Goal**: Add algorithms for detecting and removing redundant tracks and dangling segments.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 326**: PCB Dimension and Target (`pcb_dimension`).
  - **Goal**: Implement models for measuring dimensions and optical fiducial targets on the board.
  - **Status**: Completed. Merged into `ccad_core`. Phase 14 complete.
- **Sprint 325**: PCB Shape (`pcb_shape`).
  - **Goal**: Add core structs for generic PCB graphical shapes (lines, arcs, polygons).
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 324**: PCB Text and Textbox (`pcb_text`).
  - **Goal**: Add core structs for generic PCB graphical text elements and multi-line bounding boxes.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 323**: Placement Ghost Tool (`placement_ghost_tool`).
  - **Goal**: Implement logic for tracking interactive cursor state before committing a footprint to the board.
  - **Status**: Completed. Merged into `ccad_core`. Phase 13 complete.
- **Sprint 322**: Footprint Chooser Bridge (`footprint_chooser_bridge`).
  - **Goal**: Wire UI footprint library search queries to the core catalog model.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 321**: Footprint Catalog Cache (`footprint_catalog_cache`).
  - **Goal**: Add logic for indexing and searching the local footprint library cache.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 320**: Selection Filter (`selection_filter`).
  - **Goal**: Add standard UI event filtering for tracks, vias, pads, footprints, text, and zones.
  - **Status**: Completed. Merged into `ccad_core`. Phase 12 complete.
- **Sprint 319**: PCB Appearance Manager (`appearance_manager`).
  - **Goal**: Track active layers, visible layers, and layer colors independently.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 318**: Tool State Control (`tool_state_control`).
  - **Goal**: Implement centralized state machine stub for determining the active UI tool context.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 317**: Routing Tool Hooks (`routing_tool_hooks`).
  - **Goal**: Add interactive lifecycle hooks to the RouterTool for driving PNS interactively.
  - **Status**: Completed. Merged into `ccad_core`. Phase 11 complete.
- **Sprint 316**: Drawing Tool Parity (`drawing_tool`).
  - **Goal**: Implement standard interactive bridge stubs for primitive canvas drawing (lines, text).
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 315**: Selection Tool Parity (`selection_tool`).
  - **Goal**: Implement standard interactive bridge stubs for hit testing and area selection.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 314**: Router Tool Bridge (`router_tool`).
  - **Goal**: Implement standard interactive bridge stubs for the PNS algorithm.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 313**: Connectivity Data Topology Store (`connectivity_data`).
  - **Goal**: Implement standard core memory container base structure for graphing net nodes.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 312**: Eagle IO Plugin Logic (`eagle_plugin`).
  - **Goal**: Implement IO abstraction mapping for Eagle formats.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 311**: Altium IO Plugin Logic (`altium_plugin`).
  - **Goal**: Implement IO abstraction mapping for Altium formats.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 310**: Legacy IO Plugin Logic (`legacy_plugin`).
  - **Goal**: Implement IO abstraction mapping for legacy KiCad/Eagle formats.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 309**: KiCad IO Plugin Logic (`kicad_plugin`).
  - **Goal**: Implement IO abstraction linkage for `.kicad_pcb` files.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 308**: IO Manager Logic (`io_mgr`).
  - **Goal**: Implement plugin abstraction for multiple EDA formats.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 307**: Agent Settings Logic (`agent_settings`).
  - **Goal**: Implement standard disk persistence logic for environment config.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 306**: Page Layout Editor Logic (`pagelayout_editor`).
  - **Goal**: Implement standard object property logic for sheet templates.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 305**: Bitmap to Component (`bitmap2component`).
  - **Goal**: Implement standard geometric extraction logic for image footprints.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 304**: PCB Calculator Logic (`pcb_calculator`).
  - **Goal**: Implement standard IPC trace width and current capacity equations.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 303**: DRC Engine (`drc_engine`).
  - **Goal**: Implement structural mapping for the DRC orchestrator.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 302**: DRC Test Provider Edge Clearance (`drc_test_provider_edge_clearance`).
  - **Goal**: Implement structural mapping for plugin-style DRC edge clearance test.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 301**: DRC Test Provider Courtyard (`drc_test_provider_courtyard`).
  - **Goal**: Implement structural mapping for plugin-style DRC footprint courtyard overlap test.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 300**: DRC Test Provider Unrouted (`drc_test_provider_unrouted`).
  - **Goal**: Implement structural mapping for plugin-style DRC unrouted net test.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 299**: DRC Test Provider Clearance (`drc_test_provider_clearance`).
  - **Goal**: Implement structural mapping for plugin-style DRC clearance test.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 298**: DRC Test Providers (`drc_test_provider`).
  - **Goal**: Implement structural mapping for plugin-style DRC test execution.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 297**: DRC Items (`drc_item`).
  - **Goal**: Implement structural map for typed diagnostic error objects.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 296**: CVPCB Netlist Parser and Auto-Association (`read_netlist`, `auto_associate`).
  - **Goal**: Implement structural mapping from schematic netlists into automated footprint heuristics.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 295**: PNS Spatial Index (`pns_index`).
  - **Goal**: Implement structural mapping for interactive Push and Shove topology indexing.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 294**: Advanced PNS Routing Placers (`pns_diff_pair_placer`, `pns_meander_placer`).
  - **Goal**: Implement algorithms for differential pair tuning and length-matched serpentines.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 293**: PNS Routing logic implementation (`pns_dragger`, `pns_node`).
  - **Goal**: Implement structural mapping for interactive Push and Shove topology dragging heuristics.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 292**: PCB Parser Implementation (`pcb_parser`).
  - **Goal**: Implement structural mapping from S-Expression AST to native CCad Board memory models.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 291**: Deep API Bridge and Job Manager (`api_bridge`, `job_manager`).
  - **Goal**: Implement standard threaded execution and local IPC RPC dispatcher for kernel reflection.
  - **Status**: Completed. Phase 10 begins.
- **Sprint 290**: Backlog Catchup (`connection_graph`, `test_zone_intersection`, `bus_wire_junction`).
  - **Goal**: Clear the final missed stubs from the official sprint backlog.
  - **Status**: Completed. Merged into `ccad_core`. All base KiCad parity stubs from backlog and root file walk are fully accounted for.
- **Sprint 289**: API Bridge stub (`api_bridge`).
  - **Goal**: Add structural framework for JSON-RPC/IPC communication to mirror the `F:\kicad_src\api` capabilities.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 288**: Intermediate Data Format (IDF) exporter stub (`idf_exporter`).
  - **Goal**: Add structural framework for exporting 3D PCBA assemblies to MCAD from `F:\kicad_src\utils`.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 287**: Network Netlist Generator (KiNNG) stub (`kinng_base`).
  - **Goal**: Add structural framework for netlist generation from `F:\kicad_src\libs\kinng`.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 286**: KiMath and KiPlatform stubs (`kimath`, `kiplatform`).
  - **Goal**: Add structural framework for geometry math and cross-platform wrappers from `F:\kicad_src\libs`.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 285**: HTTP Client and Database Base stubs (`http_client`, `database_base`).
  - **Goal**: Add structural framework for HTTP networking and library database indexing.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 284**: Job Manager and IO Base stubs (`job_manager`, `io_base`).
  - **Goal**: Add structural framework for background multi-threading tasks and layout I/O base abstractions.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 283**: Drawing Sheet and Netlist Reader stubs (`drawing_sheet`, `netlist_reader`).
  - **Goal**: Add structural framework for title blocks and schematic netlist imports.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 282**: Font and Plotter stubs for common libraries (`font`, `plotter`).
  - **Goal**: Add structural framework for vector stroked fonts and layout plotters (PDF/SVG/Gerber).
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 281**: Graphics Abstraction Layer (GAL) base stub (`gal_base`).
  - **Goal**: Add structural framework for hardware-accelerated drawing mirroring KiCad's GAL.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 280**: SchSymbol / SchPin implementation for schematic instantiation (`sch_symbol`).
  - **Goal**: Full KiCad SCH_SYMBOL/SCH_PIN parity, including multi-unit symbols, alternate pin functions, and local library-reference snapshots.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 279**: Final ecosystem and agent settings stubs (`pagelayout_editor`, `pcb_calculator`, `bitmap2component`, `agent_settings`).
  - **Goal**: Add C++ headless utility classes for the remaining auxiliary tools.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 278**: 3rd-party EDA format importer stubs (`eda_importers`).
  - **Goal**: Add C++ headless utility classes for parsing layouts from 11 different legacy EDA tools.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 277**: Pcbnew Push and Shove router heuristics and algorithmic stubs (`pns_algo_base`, `pns_index`, `pns_node`, `pns_dragger`, `pns_diff_pair_placer`, `pns_meander_placer`).
  - **Goal**: Add C++ headless utility classes for the Push and Shove (PNS) interactive routing system.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 275**: Cvpcb auto associate footprint heuristic stubs (`auto_associate`).
  - **Goal**: Add C++ headless utility classes for footprint auto assignment heuristics.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 274**: Cvpcb schematic netlist reader and listbox stubs (`read_netlist`, `cvpcb_listboxes`).
  - **Goal**: Add C++ headless utility classes for reading legacy KiCad schematic netlists and assigning footprints.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 273**: 3D scene graph, materials, and raytracing engine stubs (`3d_scene_graph`, `3d_material`, `3d_raytracer`).
  - **Goal**: Add C++ headless utility classes for interacting with 3D hierarchies and photorealistic rendering logic.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 272**: 3D coordinate transformations and model cache stubs (`3d_math`, `3d_fastmath`, `3d_cache`, `3d_resolver`).
  - **Goal**: Add C++ headless utility classes for interacting with 3D geometry transformations and 3D asset caching.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 271**: Gerber tools for polyset translation, hit-testing and diffing stubs (`gerber_to_polyset`, `gerber_collectors`, `gerber_diff`).
  - **Goal**: Add C++ headless utility classes for interacting with Gerber datasets.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 270**: Gerber RS-274X syntax and attributes parsers stubs (`readgerb`, `rs274x`, `rs274d`, `x2_gerber_attributes`, `job_file_reader`).
  - **Goal**: Add C++ headless primitives for modeling Standard/Extended Gerber parsers and gbrjob metadata read logic.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 269**: Gerber graphic primitives and document image model stubs (`gerber_draw_item`, `gerber_file_image`, `gerber_file_image_list`).
  - **Goal**: Add C++ headless primitives for modeling Gerber document images and geometries.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 268**: Gerber manufacturing read stubs (`dcode`, `am_evaluate`, `excellon_read_drill`, `gbr_layout`).
  - **Goal**: Add headless C++ primitives for legacy Gerber parsing and drilling operations.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 267**: Gerber Aperture macro and primitive stubs (`am_param`, `am_primitive`, `aperture_macro`).
  - **Goal**: Add C++ headless primitives for modeling Gerber RS-274X aperture macros.
  - **Status**: Completed. Merged into `ccad_core`.
- **Sprint 266**: Hierarchical schematic net connectivity stubs (`sch_connection`, `sch_netchain`, `net_navigator`).
  - **Goal**: Add headless C++ primitives for processing net mapping and connectivity tracing, mimicking KiCad's approach.
  - **Status**: Completed. Merged `sch_connection`, `sch_netchain`, and `net_navigator` into `ccad_core`.
- **Sprint 265**: Schematic DXF/SVG import utilities stub (`gfx_import_utils.cpp`).
  - **Goal**: Add headless C++ primitives for processing DXF and SVG imports into Schematic Graphics, without KiCad wxWidgets dependencies.
  - **Status**: Completed. Merged `gfx_import_utils.hpp` and `.cpp` into `ccad_core`.
- **Sprint 264**: Schematic Primitive Rendering Parity (`SchText`, `SchGraphic`, `SchJunction`, `SchNoConnect`, `SchSheet`, `SchRuleArea`, etc.) into UI canvas representation.
  - **Goal**: Render schematic primitives on the Qt Canvas to achieve parity with the backend data representation.
  - **Status**: Completed. Ported primitive rendering logic to `schematic_canvas_renderer.cpp`. Resolved footprint pad parsing errors (`roundrect_rratio` and `chamfer_ratio`) during JSON load due to model updates. Verified visual output in the validation harness.
- **Sprint 263**: Schematic Symbol (SchSymbol) missing attributes serialization parity (`reference`, `unit`, `mirror_x`, `mirror_y`, `in_bom`, `on_board`, `fields`).
- **Sprint 262**: Schematic Text, Graphic, and remaining object serialization parity.
- **Sprint 261**: Schematic Sheet and Sheet Pin ID and serialization parity.
- **Sprint 260**: Schematic Junction and No Connect ID and serialization parity.
- **Sprint 258**: Schematic Group (SchGroup) Support
  - **Goal**: Replicate KiCad's `SCH_GROUP` logic into `ccad_core` to support logical groups of schematic items.
  - **Status**: Completed. Implemented `SchGroup` parsing, bounding box calculation, hit testing, and serialization. Successfully generated visual demo and merged into `main`.

## Prior Sprints
- **Sprint 256**: Schematic Item Geometry Phase 4 (Sheet Pins and Missed Tests)
  - **Goal**: Implement `SchSheetPin` item geometries (bounding box and hit test) and add missing unit tests for `SchLabel`, `SchPowerSymbol`, `SchTextBox`, `SchMarker`, `SchNoConnect`, and `SchSheetPin`.
  - **Status**: Completed. Ported bounding box and hit testing logic for `SchSheetPin`. Added comprehensive test cases for all previously missed elements to `test_item_geometry.cpp`. All tests pass cleanly, and visual validation via the harness proves no regressions.
- **Sprint 255**: Schematic Item Geometry Phase 3 (Remaining items)
  - **Goal**: Port schematic geometry functions (bounding boxes, hit tests) from KiCad for `SchPin`, `SchField`, `SchSheetPin`, `SchBitmap`, `SchBusEntry`, `SchRuleArea`, and `SchTable` into CCad's `item_geometry` layer.
  - **Status**: Completed. Ported bounding box and hit testing logic for all remaining items. Added comprehensive test cases to `test_item_geometry.cpp`. All tests pass cleanly, and visual validation via the harness proves no regressions.

## Prior Sprints
- **Sprint 254**: Schematic Text, Symbol, and Sheet Geometry
  - **Goal**: Port schematic geometry functions (bounding boxes, hit tests) from KiCad for `SchText`, `SchSymbol`, `SchSheet`, and remaining elements into CCad's `item_geometry` layer.
  - **Status**: Completed. Declarations added, tests updated, core functions ported and visually verified.
- **Sprint 253**: Schematic Line and Junction Geometry
  - **Goal**: Port schematic geometry functions (bounding boxes, hit tests, lengths) from KiCad `eeschema/sch_line.cpp` and `sch_junction.cpp` into CCad's `ccad_core/item_geometry.cpp` to unify PCB and schematic geometry capabilities.
  - **Status**: Completed. Implemented logic for `SchWire`, `SchBus`, `SchGraphic`, and `SchJunction`. Updated `item_geometry.hpp/.cpp` and `test_item_geometry.cpp`. All tests pass. Verified GUI rendering via sprint demo harness.
- **Sprint 252**: RefdesTracker Annotation Integration
  - **Goal**: Integrate the previously implemented `RefdesTracker` into `ccad_core/annotate.cpp`, replacing primitive local map assignments with the KiCad-compliant tracker logic for reference designator assignments.
  - **Status**: Completed. Replaced local `std::map<std::string, std::set<int>>` with `ccad::RefdesTracker` in `annotateSchematic` and `annotateProject`.
- **Sprint 251**: Pin Type Integration
  - **Goal**: Integrate the newly created `ElectricalPinType`, `GraphicPinShape`, and `PinOrientation` enumerations into the core CCad data models (`SchPin`, `SymbolPin`) and parsers. Replace raw string and degree fields, update JSON serialization/deserialization to match KiCad formats, and ensure tests and GUI rendering continue to work.
  - **Status**: Completed. Replaced `type`, `rotation_degrees`, and `graphical_style` strings/doubles with strongly typed enums in `symbol.hpp` and `model.hpp`. Updated `kicad_symbol_import.cpp`, `serialize.cpp`, and `symbol_json_reader.hpp` to parse and emit canonical enum strings. Fixed GUI validation test suite initialization errors with missing field warnings. All tests pass and visual validation verified at `artifacts/screenshots/sprint-demo-20260703-022143.png`.

## Prior Sprints
- **Sprint 250**: Refdes Tracker and Pin Type Parity
  - **Goal**: Port KiCad's `REFDES_TRACKER` and `pin_type.h/.cpp` logic into `ccad_core`. Provide efficient reference designator tracking with O(1) lookup, gap-filling next-available allocation, serialization/deserialization, and thread-safe operation. Provide electrical pin type, graphic pin shape, and pin orientation enums with canonical string serialization matching KiCad's format. Fix CI/CD failures (MSVC `M_PI`, Linux CLI test quoting, orchestrator test hang, dangling submodule).
  - **Status**: Completed. Implemented `ccad::RefdesTracker` in `refdes_tracker.hpp/cpp`, `ccad::ElectricalPinType`/`GraphicPinShape`/`PinOrientation` in `pin_type.hpp/cpp`. Added `test_refdes_tracker.cpp` with 3 test cases. Fixed CI blockers. All core CTest tests pass. Visual validation screenshot verified at `artifacts/screenshots/sprint-250-refdes-tracker-20260703-000801.png`.

## Prior Sprints
- **Sprint 249**: Library Symbol Inheritance
  - **Goal**: Replicate KiCad's `lib_symbol.cpp` logic to properly manage parent/child nested symbol units, aliases, and inheritance trees. Update `importKiCadSymbolLibrary` to dynamically flatten parent geometry/pins over derived symbols without breaking the standard `ccad::Symbol` data structures passed to the GUI and CLI.
  - **Status**: Completed. Implemented `ccad::LibSymbol` tree structure. Rewrote the parser loop in `kicad_symbol_import.cpp` to correctly link `.extends` parents and extract fully resolved flat symbols. Verified footprint losslessness and core symbol parsing logic passes all `ctest` harness gates.

## Prior Sprints
- **Sprint 248**: Deterministic GUI Action Tools
  - **Goal**: Extend the native GUI automation interface in `ReviewWindow` to support rich user-harness actions including double clicking, key sequences/modifiers, dragging, scrolling, dialog/menu automation, properties editing, and target validation.
  - **Status**: Completed. Implemented double-clicks on list items and canvas objects, sequence key sending with `QKeySequence`, scroll bars manipulation, dialog button and context menu clicks, and inspector properties editing; verified all 59 tests pass cleanly.

## Prior Sprints
- **Sprint 247**: Canvas Spatial Indexes
  - **Goal**: Implement a fast spatial partition grid structure to optimize nearest-object searches and hit-testing on large boards, replacing O(N) linear QGraphicsItem scans.
  - **Status**: Completed. Implemented `CanvasSpatialIndex` in `spatial_index.hpp`, integrated with `ReviewWindow::rebuildUiMapIndexCache` and `uiNearestCanvasObjectJson`, registered `ccad_gui_canvas_spatial_index_tests`, verified all 58 tests pass, and ran visual validation.
- **Sprint 246**: Unicode Path Support on Windows
  - **Goal**: Resolve command line and file stream encoding limitations on Windows causing symbol imports to fail on Unicode filenames. Intercept Unicode command line wide characters and map them safely to UTF-8.
  - **Status**: Completed. Added `filesystem_u8.hpp` with UTF-8 path helpers, updated `main.cpp` with `GetCommandLineW` and `CommandLineToArgvW` argument interception, migrated file stream calls to use `ccad::u8ToPath`, verified 100% CTest success, and confirmed visual validation harness passes.
- **Sprint 245**: KiCad Footprint Oval Drill Support
  - **Goal**: Support KiCad oval drills in the footprint importer, JSON serialization/deserialization, and footprint losslessness verification.
  - **Status**: Completed. Updated footprint structures, KiCad footprint importer parser, placement logic, footprint losslessness verification, added test cases, and confirmed all 57 tests pass. Visual validation screenshot verified successfully.
- **Sprint 244**: Footprint Losslessness Harness
  - **Goal**: Implement a footprint losslessness harness to structurally compare imported vs candidate footprints (pads, drills, shape parameters, layers, model references).
  - **Status**: Completed. Implemented verifyFootprintLosslessness, registered verify-footprint-losslessness command, added and verified tests. All tests pass. Visual validation screenshot generated.

## Backlog
- **Sprint 226 (PCB Root Model Parity)** is active. The verified sub-slices now cover KiCad board design setting validation, KiCad `BOARD_ITEM` metadata for CCad object queries, KiCad `BOARD_LOADER`-style load-state reporting, KiCad `BOARD_STACKUP` default physical stackup reporting, KiCad board-statistics drill-line aggregation, KiCad board-statistics report summaries, KiCad `BOARD_ITEM_CONTAINER` delete/remove mode metadata, KiCad `BOARD_TEXT_VAR_ADAPTER` first-slice text-variable expansion, KiCad legacy `build_BOM_from_board.cpp` board-side BOM export, KiCad `cleanup_item.cpp/.h` cleanup-action catalog discovery, GUI empty/board-only load crash regression, KiCad `collectors.cpp/.h` locked-item filtering, CLI project-write truncation hardening, KiCad `convert_shape_list_to_polygon.cpp/.h` Edge.Cuts outline-polygon reporting, KiCad `cross-probing.cpp` packet resolution, KiCad `pcb_barcode.cpp` board-barcode IO/CLI/GUI parity, KiCad `pcb_dimension.cpp` and `pcb_group.cpp` IO/CLI/GUI parity, and KiCad reference-image and table rendering with GUI infinite-loop hardening before moving to the next root PCB editor file.
- **Sprint 225 (Parity Backlog Normalization and Orchestrator Continuity)** normalized `docs/devops/backlog.md` and this progress file so the user's KiCad file-by-file parity directive became a sprint-bounded roadmap with explicit PCB editor, schematic editor, external format, Gerber, 3D, multi-document project, library verification, GUI-map, agent harness, autorouter, and branch-cleanup tracks.
- **Sprint 224 (KiCad Board Bounding Box, Commit Impact, and Connected Items)** is in progress on `sprint-224-kicad-board-bounding-box`. The slice adds a first explicit CCad analogue for KiCad's `BOARD_BOUNDING_BOX` wrapper: it is a transient, skip-struct, GUI-independent `CanvasScene` view item on `LAYER_BOARD_BOUNDING_BOX`, and `pcb get-outline` reports the same class, view-layer, skip-struct, and bounding-box payload for agents. It also maps the useful headless part of KiCad's `BOARD_COMMIT` into `Transaction::impact`, reporting board/schematic changes, view, DRC, ERC, connectivity, ratsnest, board-outline, solder-mask, dirty object ID, and dirty object type metadata in audit JSON. The current connected-item slice maps KiCad `BOARD_CONNECTED_ITEM` into pads, vias, tracks, and zones returned by `pcb list-objects`, `pcb list-by-net`, `pcb list-connected`, object lookup, and route-job export, including net-name fields, local ratsnest visibility, teardrop capability hints, and an explicit default-netclass fallback until CCad has a full netclass model.
- **Sprint 223 (KiCad Board Document Model Parity)** is verified on `sprint-223-kicad-board-model`. The slice removes unsafe schematic assumptions from core and CLI behavior, adds primary document helpers, lets board-only projects run physical DRC without irrelevant schematic diagnostics, preserves board-local nets in KiCad PCB export, keeps footprint placement board-owned, creates schematic documents for symbol/schematic commands, and makes review, BOM, PnP, diff, and agent context tolerate unlinked boards. The whole-tree diff check passed, the final incremental Qt build passed, and the final sprint-end CTest gate passed 41 of 41 tests.
- **Sprint 222 (KiCad PCB API Items, Matrix, Autoplace, and Spread)** is verified on `sprint-222-kicad-pcb-api-items`. The slice records the full KiCad PCB API handler, enum, `BOARD_CONTEXT`, and `HEADLESS_BOARD_CONTEXT` ledgers in `agent pcb-api-schema`; adds a KiCad `array_pad_number_provider` analogue for deterministic pad numbering; adds a first `ar_matrix` analogue for grid occupancy, side masks, distance maps, and keepout costs; improves footprint autoplacement with the matrix cost field; exposes `pcb autoplace-footprint`; adds a first `SpreadFootprints` analogue for deterministic non-overlapping component pad-group spreading; and exposes `pcb spread-footprints`. Focused tests are green, the full Qt build passed, CTest passed 41 of 41, and commit remains pending.
- **Sprint 221 (KiCad PCB API Utility Layer Sets)** is complete on `sprint-221-kicad-pcb-api-utils`. The slice maps KiCad `PackLayerSet` / `UnpackLayerSet` behavior into CCad's current model by resolving KiCad wildcard selectors such as `*.Cu` and `*.Mask` against the active board layer list, preserving unmatched concrete selectors for validation, emitting canonical KiCad layer numbers for resolved layer sets, expanding placed footprint pad layers, and exposing resolved pad layer metadata through `pcb list-objects`, `pcb export-route-job`, `pcb list-by-net`, and `pcb list-connected`. Focused layer, placement, and CLI checks are green. The sprint-end full build and CTest gate passed 37 of 37, and the official visual harness produced `artifacts/screenshots/sprint221-kicad-layer-set-utils-20260611-182341.png`.
- **Sprint 220 (KiCad PCB API Parity Audit, Slice A)** is complete on `sprint-220-kicad-pcb-parity-audit`. It maps KiCad PCB handlers such as `GetBoardEnabledLayers`, `GetVisibleLayers`, `GetBoardLayerName`, `GetBoardStackup`, `GetBoardDesignRules`, `GetBoundingBox`, `GetItemsByNet`, and `GetConnectedItems` to deterministic CCad CLI and Agent metadata surfaces. The slice adds ten agent-usable PCB query capabilities, fixes the Agent orchestrator's default provider-plan status so disabled provider execution is blocked by contract, and records the remaining gap as full KiCad connectivity, board-origin, graphics-defaults, pad-shape-as-polygon, net-class, selection, active-layer mutation, and custom-rule parity. Sprint-end verification passed the full Qt build plus 37 of 37 CTest tests.
- **Sprint 218 (Parity)** is complete. Implemented full end-to-end tool parity between the PCB Editor and Schematic Canvas. Schematics can now be dragged, hotkey deleted, context menu accessed identical to PCB tools. Backend mock tools in orchestrator have been completely replaced with functional `ToolNode` calls.
- **Sprint 217 (Marketplace & Slash Command UI)** is complete. Implemented a live HTTP JSON marketplace, connected agent controls to orchestrator stdin, and added the skeleton slash command parser.
- **Sprint 215 (Multi-Agent Refinement & UI Overhaul)** is complete. It introduced a Supervisor-based Multi-Agent system to `orchestrator.py` with Router and Librarian sub-agents, completed a premium UI redesign of `agent_panel.cpp`, and isolated the Python process in a dedicated `venv`.
- **Sprint 211 (Python Orchestrator Transition)** is complete on `sprint-211-python-orchestrator-chat-ui`. It initializes the `src/ccad_agent/` directory with `requirements.txt` for `langgraph`, `langchain`, `langfuse`, and `langsmith`. It authors `orchestrator.py` defining the initial `StateGraph` skeleton. The C++ `AgentOrchestrator` was cleaned up by removing deterministic planning (Sprint 210 overhaul), moving toward IPC JSON-RPC integration. The Qt GUI and tests successfully built and passed 37 of 37 tests.
- **Sprint 210 (Orchestrator Architecture Overhaul)** is complete. It refactored `src/ccad_core/agent_orchestrator.hpp/.cpp` to remove deterministic planning logic and introduced `IntakeLayer`, `ToolBroker`, `ContextBuilder`, and `Subagent` C++ stubs to prepare for the Python backend JSON-RPC integration.
- **Sprint 209 (Agent Chat UI Redesign)** is complete on `sprint-209-agent-chat-ui`. It replaced the dense vertical orchestrator 'lane' UI with a unified, chat-focused interface as requested by user wireframes. It removes legacy diagnostic/approval lane logic, implements the base layout for a chat history area, and adds the skeleton `AgentMarketplaceDialog` to house future integrations. The sprint-end Qt build plus CTest gate passed 37 of 37 tests (legacy test suites stubbed), and visual validation produced `artifacts/screenshots/sprint-demo-*.png` verifying the structure matches the hand-drawn layout.
- **Sprint 205 (Agent Provider Controls GUI Binding)** is complete on `sprint-205-agent-provider-controls-gui-binding`. It binds the native right-side Agent pane to the existing no-secret provider metadata contract without enabling provider execution, network probes, browser-account automation, secret capture/display, telemetry export, or project mutation. The pane now exposes targetable provider widgets `panel:agent_provider_controls`, `control:agent_provider_family`, `control:agent_provider_model`, `action:agent_provider_refresh_status`, `label:agent_provider_status`, `label:agent_provider_env`, and `label:agent_provider_execution_status`. `agent.workspace_state` reports `provider_panel_available`, `provider_id`, `provider_label`, `provider_access_path`, `provider_model_hint`, `provider_model_env`, `provider_api_key_env`, `provider_env_vars`, `provider_env_present`, `provider_configured`, `provider_status`, `provider_status_method`, `provider_config_schema_method`, `provider_config_template_method`, `provider_execution_enabled:false`, `provider_secret_value_visible:false`, `provider_network_probe_enabled:false`, `provider_browser_account_automation_enabled:false`, `provider_project_file_secret_storage:false`, `provider_secret_value_policy:"never_emit_secret_values"`, and `provider_readiness_policy:"env_presence_only_no_network_probe"`. The UI-map layer now exports and validates targetable `QComboBox` controls, semantically focuses provider-family controls, and allows `ui.type_text` to set the provider model hint. Focused GUI coverage passed 2 of 2 tests after red tests proved the missing provider UI and map contract. Official visual validation produced and was inspected at `artifacts/screenshots/sprint205-agent-provider-controls-gui-binding-final-20260605-150230.png`; target validation produced `artifacts/screenshots/sprint205-agent-provider-controls-gui-binding-targets-target-sequence.json` with empty stderr, no `found:false` entries, and visible initial/resized provider targeting. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 204 (Agent Trace Links GUI Binding)** is complete on `sprint-204-agent-trace-links-gui-binding`. It binds the native right-side Agent pane to local trace-link metadata without enabling provider execution, OpenTelemetry export, Langfuse network calls, prompt/tool payload capture, secret storage, or project mutation. The pane now exposes targetable trace widgets `panel:agent_trace_links`, `label:agent_trace_id`, `label:agent_span_id`, `label:agent_trace_status`, `label:agent_trace_export_status`, and `action:agent_new_trace_context`. `agent.workspace_state` reports `trace_id`, `span_id`, `trace_status`, `trace_export_status`, `trace_backend:"local_metadata_only"`, `trace_export_enabled:false`, `trace_link_available:false`, `trace_session_id`, `trace_thread_id`, and `trace_content_policy:"metadata_only_no_prompt_tool_or_design_payloads"`. The UI target resolver now scrolls targetable controls and panels inside scroll areas before returning coordinates, which prevents negative offscreen Agent-pane coordinates during target automation, and canvas-object validation samples real hit points inside object bounds instead of trusting only bounding-box centers. Focused GUI coverage passed 14 of 14 tests. Official visual validation produced and was inspected at `artifacts/screenshots/sprint204-agent-trace-links-gui-binding-final2-20260605-140444.png`; target validation produced `artifacts/screenshots/sprint204-agent-trace-links-gui-binding-targets-final-target-sequence.json` with empty stderr and visible initial/resized trace-action targeting. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 203 (Agent Policy GUI Binding)** is complete on `sprint-203-agent-policy-gui-binding`. It moves the Agent command policy classifier into `ccad_core` so the CLI, GUI, and future runner share one read/write/dry-run contract, while `src/ccad_cli/agent_policy.hpp/.cpp` remains a compatibility surface for existing CLI call sites. The native right-side Agent pane now exposes targetable policy controls `panel:agent_policy_surface`, `label:agent_policy_decision`, `label:agent_policy_risk`, `control:agent_policy_dry_run`, and `action:agent_policy_preview`; `agent.workspace_state` reports `policy_decision`, `policy_risk_level`, `policy_approval_required`, `policy_approval_reason`, `policy_dry_run`, `policy_would_execute`, `policy_read_only`, `policy_mutates_project`, `policy_mutates_files`, `policy_command`, and `policy_args`. Staged read commands such as `help --format json` classify as low-risk `allow_read`, project-mutating commands such as `pcb add-via --file board.ccad.json` classify as `approval_required` with `project_mutation`, and dry-run preview reports `dry_run_only` without execution. The GUI UI-map layer now exports, validates, resolves, and semantically clicks `control:` `QCheckBox` widgets so the dry-run checkbox is visible to the same harness that drives other controls. Focused GUI coverage passed 14 of 14 tests and `agent_serve` passed after red tests proved the missing policy surface and missing checkbox target. Official visual validation produced and was inspected at `artifacts/screenshots/sprint203-agent-policy-gui-binding-20260605-124953.png`; target validation produced `artifacts/screenshots/sprint203-agent-policy-gui-binding-targets-v2-target-sequence.json` with no `false` entries and empty stderr, including initial and resized screenshots for `control_agent_policy_dry_run`. The sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 202 (Agent Session GUI Binding)** is complete on `sprint-202-agent-session-gui-binding`. It binds the native right-side Agent pane to CCad's existing local `.ccad-agent-session.json` contract without enabling provider execution, telemetry export, project mutation, or secrets handling. The GUI now has targetable local session controls `panel:agent_session_binding`, `control:agent_session_path`, `action:agent_load_session`, `action:agent_checkpoint_session`, and `label:agent_session_status`. `agent.workspace_state` reports `durable_session_bound`, `session_file_path`, `session_path_input`, `durable_session_id`, `thread_id`, `checkpoint_count`, `latest_checkpoint_id`, `replayable`, and `session_status` while preserving the Sprint 201 visual contract. Focused GUI coverage passed 14 of 14 tests after a red test proved the missing durable-session contract. Official visual validation produced and was inspected at `artifacts/screenshots/sprint202-agent-session-gui-binding-v2-20260605-122454.png`; target validation produced `artifacts/screenshots/sprint202-agent-session-gui-binding-v2-targets-target-sequence.json` with no `false` entries and empty stderr; the sprint-end Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 201 (Agent Panel Visual Refinement)** is complete on `sprint-201-agent-panel-visual-refinement`. It refines the native right-side Agent pane toward the provided vertical agent-panel references without enabling provider execution, telemetry export, or project mutation. The GUI keeps existing semantic IDs and adds targetable `panel:agent_status_rail`, `panel:agent_command_composer`, `panel:agent_plan_deck`, `panel:agent_evidence_lane`, and `panel:agent_approval_lane` subregions. `agent.workspace_state` now reports `visual_style:"agent_reference_panel_v4"`, `workspace_layout_version:4`, and visible-section entries for the new regions while preserving run state, trace/session labels, activity events, evidence cards, approvals, command text, and plan items. Focused GUI coverage passed 14 of 14 tests after a red test proved the missing style/version and new targetable regions. Official visual validation produced and was inspected at `artifacts/screenshots/sprint201-agent-panel-visual-refinement-v2-20260605-115850.png`; target validation produced `artifacts/screenshots/sprint201-agent-panel-visual-refinement-v2-targets-target-sequence.json` with no `false` entries and empty stderr; the sprint-end Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 200 (KiCad CLI Evidence Integration)** is complete on `sprint-200-kicad-cli-evidence`. It adds the first headless KiCad evidence surface for agents with direct `ccad agent kicad-evidence-schema`, `kicad-evidence-plan`, `kicad-evidence-dry-run`, and guarded `kicad-evidence-run` commands plus JSON-RPC routes `agent.kicad_evidence_schema`, `agent.kicad_evidence_plan`, `agent.kicad_evidence_dry_run`, and `agent.kicad_evidence_run`. The sprint maps official KiCad CLI DRC/ERC/export behavior into structured command arrays for `pcb drc`, `sch erc`, `pcb export gerbers`, `pcb export drill`, `pcb export pos`, `pcb export ipc2581`, and `pcb export odb`; reports artifact manifests, readiness, executable/source decisions, and no-network/no-secret policy; classifies explicit KiCad execution as `external_process_file_write`; and denies JSON-RPC `execute:true` KiCad evidence runs from read-only `agent serve` with error `-32604`. Focused `agent_serve` coverage passed after red tests proved the missing catalog and JSON-RPC execution guard. Direct CLI spot checks returned schema, plan, dry-run, guarded run, policy-check, help, and tool-guide metadata. Official visual validation produced `artifacts/screenshots/sprint200-kicad-cli-evidence-final-20260605-114425.png`, target validation produced `artifacts/screenshots/sprint200-kicad-cli-evidence-final-targets-target-sequence.json` with no `false` entries and empty stderr, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 199 (Agent Panel UI Polish)** is complete on `sprint-199-agent-panel-ui-polish`. It upgrades the native right-side Agent pane into a denser vertical command surface with local run controls, trace/session chips, and first-viewport active-plan rows. The sprint keeps provider execution, remote telemetry, and project mutation out of these controls; the pause/resume/stop buttons update local GUI state, the Activity stream, and `agent.workspace_state` only. `agent.workspace_state` now reports `visual_style:"agent_command_center_dense"`, `workspace_layout_version:3`, `run_state`, `run_label`, `trace_label`, `session_label`, and bounded `plan_items`, while the UI map exposes `panel:agent_trace_strip`, `label:agent_trace_chip`, `label:agent_session_chip`, `panel:agent_run_controls`, `label:agent_run_state_chip`, `action:agent_pause_run`, `action:agent_resume_run`, `action:agent_stop_run`, `panel:agent_active_plan`, and `panel:agent_plan_row_1` through `panel:agent_plan_row_3`. Focused GUI coverage passed 14 of 14 tests. Official visual validation produced and was inspected at `artifacts/screenshots/sprint199-agent-panel-ui-polish-final-20260605-110500.png`, target validation produced `artifacts/screenshots/sprint199-agent-panel-ui-polish-targets-target-sequence.json` with all initial and resized targets found and empty stderr, `git diff --check` passed with only normal CRLF warnings, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 198 (Agent Observability Configuration)** is complete on `sprint-198-agent-observability-config`. It adds headless disabled-by-default trace export configuration metadata through direct `ccad agent trace-export-schema`, `trace-export-template`, `trace-redaction-policy`, and `trace-export-dry-run` commands plus JSON-RPC routes `agent.trace_export_schema`, `agent.trace_export_template`, `agent.trace_redaction_policy`, and `agent.trace_export_dry_run`. The sprint keeps actual telemetry export disabled, performs no network probe, prints no OTLP header values, records OpenTelemetry GenAI and Langfuse reference behavior, and routes the tool guide to `headless_cli_observability_config`. Focused red/green `agent_serve` coverage passed after proving the missing trace method catalog, direct CLI spot checks returned schema/template/redaction/dry-run/tool-guide/help metadata, `git diff --check` passed with only normal CRLF warnings, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 197 (Agent BYOK Configuration)** is complete on `sprint-197-agent-byok-config`. It adds headless no-secret provider configuration metadata for OpenAI, OpenAI-compatible APIs, Anthropic, Google Gemini, and local model servers through direct `ccad agent provider-config-schema`, `provider-config-template`, and `provider-status` commands plus JSON-RPC routes `agent.provider_config_schema`, `agent.provider_config_template`, and `agent.provider_status`. The sprint keeps provider execution disabled, stores no API keys in project files, reports only environment-variable presence booleans, and records Gemini key-restriction guidance from current Google docs. Focused red/green `agent_serve` coverage passed after proving the missing catalog method, direct CLI spot checks returned the schema/template/status/tool-guide/help metadata, `git diff --check` passed with only normal CRLF warnings, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 196 (Agent Workspace Command Center)** is complete on `sprint-196-agent-workspace-command-center`. It redesigns the right-side Agent pane into a denser command-center surface inspired by the provided agent UI references: a compact session strip, visible icon header actions, model/mode/local-policy chips, Command/Evidence/Approvals selectors, a first-viewport activity stream, and a fixed command bar. It keeps provider execution out of the pane, preserves the existing task/evidence/approval/command controls, adds `visual_style:"command_center_dark"`, `workspace_layout_version:2`, `visible_sections`, and `activity_events` to `agent.workspace_state`, and exposes passive Agent regions such as `panel:agent_session_strip`, `panel:agent_mode_strip`, `panel:agent_activity_stream`, `tab:agent_command`, `tab:agent_evidence`, `tab:agent_approvals`, and `label:agent_permission_chip` through the UI map and target resolver. Focused `gui_agent_panel` and `gui_ui_map` tests passed after red tests proved the missing command-center contract and UI-map regions. Official visual validation produced `artifacts/screenshots/sprint196-agent-workspace-command-center-v2-20260605-094606.png`, target validation produced `artifacts/screenshots/sprint196-agent-workspace-command-center-v2-targets-target-sequence.json` with no `found:false` entries and empty stderr, and the sprint-end full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 195 (Agent Evidence Manifests)** is complete on `sprint-195-agent-evidence-manifests`. It adds structured native Agent evidence cards for screenshot, DRC, ERC, diagnostics, and generic tool outputs; exposes `evidence_cards` through `agent.workspace_state`; updates `agent.evidence_manifest_schema` in both GUI and headless CLI metadata; keeps legacy evidence summaries for compatibility; moves Pinned Evidence above Approval Pending in the vertical Agent pane; and updates the target-sequence harness to click Trigger DRC and Pin through semantic `ui.click` controls before screenshotting the card state. Focused `agent_serve`, `gui_agent_panel`, and `gui_ui_map` tests passed after red tests proved the missing evidence-card schema and the hidden evidence-tray layout bug. Official visual validation produced `artifacts/screenshots/sprint195-agent-evidence-manifests-final-v2-20260605-042859.png`, target validation produced `artifacts/screenshots/sprint195-agent-evidence-manifests-targets-final-target-sequence.json` plus pinned-card screenshots before and after resize, `git diff --check` passed, and the full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 194 (Agent Policy Gates)** is complete on `sprint-194-agent-policy-gates`. It adds the first deterministic Agent command policy classifier with direct `ccad agent policy-schema`, `ccad agent policy-check`, and `ccad agent dry-run` commands; JSON-RPC routes `agent.policy_schema` and `agent.policy_check`; method-catalog, quickstart, harness-context, tool-guide, and help discovery; dry-run decisions that do not execute writes; and centralized `agent serve` permission checks with approval-oriented error messages. Focused `agent_serve` coverage passed after a red test proved the missing policy routes, direct CLI spot checks returned schema, approval-required write-policy, and dry-run decisions, `git diff --check` passed, and the full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 193 (Agent Session Checkpoints)** is complete on `sprint-193-agent-session-checkpoints`. It adds provider-free local Agent session files with `ccad agent session-schema`, `ccad agent session-new`, `ccad agent session-state`, `ccad agent checkpoint-add`, and `ccad agent replay`; read-only JSON-RPC routes `agent.session_schema`, `agent.session_state`, and `agent.replay_manifest`; explicit `session_id`/`thread_id` continuity; local resource URIs for sessions and checkpoints; ordered checkpoint records; and replay manifests. Focused `agent_serve` coverage passed after a red test proved the missing catalog routes, CLI spot checks created and replayed a real local session file, `git diff --check` passed, and the full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 192 (Agent CLI Workspace State)** is complete on `sprint-192-agent-cli-workspace-state`. It adds headless CLI parity for the first Agent workspace state slice with direct `ccad agent state`, `ccad agent tasks`, `ccad agent evidence`, and `ccad agent approvals` commands; matching JSON-RPC routes `agent.state`, `agent.workspace_state`, `agent.tasks`, `agent.evidence`, and `agent.approvals`; method-catalog and tool-guide discovery for those state surfaces; and explicit `durable_store:"not_configured"` fields so the CLI does not pretend to read GUI-only in-memory state before Sprint 193 durable sessions. Focused `agent_serve` coverage passed after a red test proved the missing routes, CLI spot checks returned deterministic JSON, `git diff --check` passed, and the full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 191 (Agent Panel Vertical Workspace)** is complete on `sprint-191-agent-panel-vertical-workspace`. It moves the Agent dock from a stacked lower right pane into a full-height right-side workspace column beside Layers / Objects, redesigns the panel into a compact dark workspace with session header, workspace context, goal/task area, approval card, fixed command prompt, visible Request Context and Trigger DRC actions, command staging state, and preserved semantic IDs for existing agent tools. Focused `gui_agent_panel` and `gui_ui_map` tests passed, official visual validation produced `artifacts/screenshots/sprint191-agent-panel-vertical-workspace-v6-20260605-031347.png`, target validation produced `artifacts/screenshots/sprint191-agent-panel-vertical-workspace-v6-targets-target-sequence.json` with no `found:false` entries and empty stderr, `git diff --check` passed, and the full Qt build plus CTest gate passed 36 of 36 tests.
- **Sprint 190 (Agent Harness Backlog Map)** is complete on `sprint-190-agent-harness-backlog-map`. It ingested `docs/req_agentHarness.md` into a structured backlog map, recorded external references for current agent UI, durable execution, MCP, OpenTelemetry/Langfuse, KiCad CLI, KiCad/ngspice simulation, and Altium automation, corrected the stale progress counter, and defined the next bounded agent-harness sprint sequence. `git diff --check` passed, the Qt build passed, and CTest passed 36 of 36 tests.
- **Sprint 148 (KiCad GUI Parity)** is complete and merged to `main`. It added KiCad-style icon toolbars, GUI footprint/symbol placement flows, and shape-aware pad/drill rendering.
- **Sprint 149 (Pad Shape and Layer Fidelity)** is complete and merged to `main`. It carries KiCad pad shape metadata through import, placement, project JSON, KiCad export, diffs, and GUI rendering.
- **Sprint 150 (Standard Layer Registry Fidelity)** is complete and merged to `main`. It aligns CCad's standard layer order and KiCad PCB export layer numbers with current KiCad source and exposes canonical layer numbers to agent-facing layer queries.
- **Sprint 151 (KiCad Pad Authoring CLI)** is complete and merged to `main`. It exposes KiCad-style pad type, shape, drill, roundrect ratio, chamfer ratio, and multi-layer authoring through `ccad pcb add-pad` and `ccad pcb set-pad`.
- **Sprint 152 (Rich Pad Query Contract)** is complete and merged to `main`. It exposes KiCad-style pad metadata through `ccad pcb get-object` and `ccad pcb list-objects --type pad` so agents can plan from command output instead of parsing raw project JSON.
- **Sprint 153 (Route-Job Pad Metadata)** is complete and merged to `main`. It carries KiCad-style pad type, shape, drill, ratio, layer, and rotation metadata into `ccad pcb export-route-job` so external routers and AI tools see the same pad geometry intent.
- **Sprint 154 (GUI Actions and Library Cache Placement)** is complete and merged to `main`. It replaced top-toolbar placeholders with real Save, Board Setup, Undo, Redo, Run DRC, and DRC export behavior, made the library chooser more KiCad-like, and fixed converted symbol inheritance so library-cache symbol placement gets real pins.
- **Sprint 155 (KiCad Placement Chooser and Ghost Placement)** is complete and merged to `main`. It removed raw file-preview workflows from the user-facing placement path, routed Add Symbol/Add Footprint by active editor tab, used local `library-cache` chooser data, added cursor-following placement ghosts, and fixed layer color and selected-track visibility problems.
- **Sprint 156 (Lazy Library Chooser Loading)** is complete on `sprint-156-lazy-library-chooser`. It fixes Add Symbol/Add Footprint hangs by indexing `library-cache` entries cheaply and deferring actual symbol or footprint parsing until a row is selected for details/preview or placed.
- **Sprint 157 (Read-only GUI UI Map)** is complete on `sprint-157-readonly-ui-map`. It adds an app-owned semantic UI map dump and live target validation for future LLM/native automation tooling, with stable action IDs, tab and canvas bounds, canvas object metadata, route/layer/net context, and target coordinates proven by Qt hit-testing.
- **Sprint 158 (UI Target Queries)** is complete on `sprint-158-ui-target-queries`. It adds app-owned targeted coordinate queries by semantic ID and board-space point so agents can request one actionable target without dumping the full UI map.
- **Sprint 159 (Safe UI Actions)** is complete on `sprint-159-safe-ui-actions`. It adds app-owned direct triggering for a small allowlist of safe view/navigation actions and explicit refusal for unsafe or mutating actions.
- **Sprint 160 (Placement Crash, CI Fixes, Icon Recovery, and Backlog Ledger)** is complete and merged to `main`. It fixes left-click placement reload crashes, CI compile failures in KiCad symbol import and DSN export, robust KiCad SVG icon lookup, app-owned placement-click regression coverage, and the consolidated backlog ledger.
- **Sprint 161 (UI Map Mouse Target Harness)** is complete and merged to `main`. It adds menu/panel nodes to the semantic UI map and an app-owned mouse-target screenshot harness with beep, fast policy waits, marker overlays, and resize-repeat evidence.
- **Sprint 162 (Pad Layer Rendering Fidelity)** is complete and merged to `main`. It separates copper, solder-mask, and solder-paste pad rendering across the board canvas, footprint chooser preview, and footprint placement ghost.
- **Sprint 163 (Live UI Map Server)** is complete and merged to `main`. It adds a local socket JSON Lines server for repeated `ui.map`, `ui.target`, and `ui.epoch` queries while the GUI stays open.
- **Sprint 164 (Toolbar Action Contracts)** is complete on `sprint-164-toolbar-action-contracts`. It removes silent right-toolbar stubs by giving unfinished editor tools visible planned-tool status and an agent-readable `future_tool_not_implemented` safe-trigger response.
- **Sprint 165 (Left Toolbar Action Contracts)** is complete on `sprint-165-left-toolbar-action-contracts`. It extends the no-silent-stubs contract to unfinished left-toolbar display and panel controls.
- **Sprint 166 (Left Toolbar Real Toggles)** is complete on `sprint-166-left-toolbar-real-toggles`. It turns Show Layers and Show Properties into real panel toggles with `panel_toggled` safe-trigger responses.
- **Sprint 167 (Native Agent Panel Shell)** is complete on `sprint-167-agent-panel-shell`. It adds the first persistent Qt Agent panel for UI-map refreshes, safe UI action triggers, and `panel:agent` targeting without provider or secret integration. It also updates the GUI visual-validation timing policy to 7 seconds for single-preview screenshots and 5 seconds initial plus 800 ms per-target waits for multi-target GUI harness runs.
- **Sprint 168 (Left Toolbar Display Controls)** is complete on `sprint-168-left-toolbar-display-controls`. It turns grid, polar coordinates, inch units, crosshair, ratsnest, net highlight, and display mode into real safe display actions, exposes checked state in the UI map, extends the target-sequence harness to exercise those controls, and fixes stale inspector editor overlap during rapid multi-selection changes.
- **Sprint 169 (Visual Harness Timing Policy)** is complete on `sprint-169-visual-harness-timing`. It enforces the updated 7-second single-preview timing and 5-second initial plus 800 ms per-action multi-target timing through a dedicated CTest policy guard, updates the live interaction wrapper, and fixes chooser-row targeting so live footprint preview checks actually select a catalogue row.
- **Sprint 170 (PCB Edit Tool Entry)** is complete on `sprint-170-pcb-edit-tool-entry`. It promotes Add Via, Route Track, Add Keepout, and Delete from right-toolbar planned-tool status to real kernel-backed GUI edit entries with matching agent/test hooks, while keeping Add Zone, Draw Graphic, and Place Text as explicit planned tools until their durable board primitives exist. Focused GUI-map coverage passed, official visual validation produced a full bridge-rectifier screenshot, app-owned viewport hooks placed a via, track, and keepout before deleting the new via, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 171 (PCB Active Layer Context)** is complete on `sprint-171-pcb-active-layer-context`. It adds a top-toolbar active copper-layer selector, agent-readable/settable active-layer JSON, live UI-map socket active-layer methods, and active-layer-aware footprint placement plus Route Track commits. Focused GUI-map coverage passed, official visual validation produced `artifacts/screenshots/sprint171-pcb-active-layer-context-final-20260603-010814.png`, app-owned active-layer hooks returned `F.Cu`, set `B.Cu`, and resolved `control:active_pcb_layer`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 172 (PCB Active Net Context)** is complete on `sprint-172-pcb-active-net-context`. It adds a top-toolbar active net selector, agent-readable/settable active-net JSON, live UI-map socket active-net methods, `control:active_pcb_net` targeting, and active-net-aware Add Via plus Route Track commits. Focused GUI-map coverage passed, official visual validation produced `artifacts/screenshots/sprint172-pcb-active-net-context-final-20260603-012426.png`, app-owned active-net hooks returned `AC1`, set `DC_NEG`, and resolved `control:active_pcb_net`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 173 (PCB Graphics and Text Tools)** is complete on `sprint-173-pcb-graphics-text-tools`. It adds durable
  - `pcb_textbox.cpp` -> CCad has `BoardText` but no full textbox yet.
  - `pcb_text.cpp` -> Done (Phase 2, sprint `sprint-211-text-variables`)
  - `pcb_track.cpp` -> Done (Phase 2, `BoardTrackSegment`)
  - `pcb_target.cpp` -> Done (Phase 2, `BoardTarget`, `add-target`, JSON parity and Qt canvas rendering)
  - `pcb_barcode.cpp` -> Done (Phase 2, `BoardBarcode`, `add-barcode`, JSON parity and Qt canvas rendering)
  - `pcb_shape.cpp` -> Done (Phase 2, `BoardGraphic`, `CanvasArc`/`CanvasCircle`/`CanvasPolygon`) and `BoardText` primitives, CLI authoring and compact queries, DRC/diff/KiCad PCB export support, real Draw Graphic and Place Text GUI edit modes, object-browser and inspector support, hidden-layer-safe defaults, and the official bridge-rectifier visual proof at `artifacts/screenshots/sprint173-pcb-graphics-text-tools-final-20260603-021600.png`. Focused tests passed, the official visual screenshot was inspected, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 174 (PCB Zone Tool First Slice)** is complete on `sprint-174-pcb-zone-tool`. It adds durable KiCad-compatible first-slice `BoardZone` primitives with CLI `pcb add-zone`, compact query/list and route-job exposure, DRC/diff/KiCad PCB export support, real Add Zone GUI edit mode, object-browser and inspector support, app-owned zone placement through the Qt viewport event path, and the official bridge-rectifier zone visual proof at `artifacts/screenshots/sprint174-pcb-zone-tool-final-20260603-031321.png`. Focused tests passed, the official visual screenshot was inspected, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 175 (Agent Panel Live UI Map)** is complete on `sprint-175-agent-panel-live-map`. It connects the native Agent panel to the same live UI-map protocol as the local socket, adds `ui.map_delta`, `ui.find`, `ui.target_board_point`, and `ui.trigger_safe` live methods, exposes Agent method/payload/action controls as targetable UI-map nodes, parses live socket requests with Qt JSON APIs, and ingests `docs/req_agentHarness.md` into the local backlog. Focused tests passed, official visual validation produced `artifacts/screenshots/sprint175-agent-panel-live-map-final-20260603-102936.png`, the target harness proved Agent controls in `artifacts/screenshots/sprint175-agent-panel-live-map-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 176 (UI Map Compact Deltas)** is complete on `sprint-176-ui-map-compact-deltas`. It adds `ui.map_compact`, `ui.role_summary`, `ui.hit_test`, and `ui.nearest_canvas_object` to the shared Agent-panel/live-socket dispatcher, changes stale `ui.map_delta` responses to compact nodes instead of full nested maps, keeps current-epoch deltas empty, and covers direct plus socket routing in GUI-map tests. Focused tests passed, official visual validation produced `artifacts/screenshots/sprint176-ui-map-compact-deltas-final-20260603-110523.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint176-ui-map-compact-deltas-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 177 (Agent UI Interaction Tools)** is complete on `sprint-177-agent-ui-interaction-tools`. It adds the first direct semantic interaction tools on top of the shared Agent-panel/live-socket dispatcher: dry-run click targeting, safe action/tab clicks, Agent-control focus and text entry, Escape cancellation, PCB canvas-object selection, selection inspection, and bounded UI epoch waits. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint177-agent-ui-interaction-tools-final-20260603-112907.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint177-agent-ui-interaction-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 178 (Map-Driven Viewport Input)** is complete on `sprint-178-map-driven-viewport-input`. It adds `ui.canvas_click` and `ui.canvas_drag` to the shared Agent-panel/live-socket dispatcher so agents can resolve PCB board-space coordinates and send real Qt viewport mouse events through the same GUI tool event path as human clicks. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint178-map-driven-viewport-input-final-20260603-115354.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint178-map-driven-viewport-input-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 179 (Agent PCB Workflow Tools)** is complete on `sprint-179-agent-pcb-workflow-tools`. It adds higher-level Agent-panel/live-socket workflow methods `ui.current_tool`, `ui.cancel_tool`, `ui.place_via`, `ui.route_track`, `ui.add_zone`, `ui.add_keepout`, `ui.draw_graphic`, `ui.place_text`, and `ui.delete_object`, all composed from safe action activation plus real viewport input or canvas-object selection. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint179-agent-pcb-workflow-tools-final-20260603-121454.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint179-agent-pcb-workflow-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 180 (Agent Project Evidence Tools)** is complete on `sprint-180-agent-project-evidence-tools`. It adds `ui.screenshot`, `project.context`, `project.object_counts`, `project.review`, `project.erc`, `project.drc`, and `project.diagnostics` to the shared Agent-panel/live-socket dispatcher so agents can capture app-owned GUI evidence and query loaded-project health without shelling out or scraping files. Focused `gui_ui_map`, `gui_agent_panel`, and `review` tests passed, official visual validation produced `artifacts/screenshots/sprint180-agent-project-evidence-tools-final-20260603-123654.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint180-agent-project-evidence-tools-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 181 (UI Map Dirty Deltas and Canvas Index)** is complete on `sprint-181-ui-map-dirty-index`. It upgrades the Agent-panel/live-socket UI-map surface with first-slice dirty semantic IDs, changed roles, bounded `ui.wait_for_delta`, and indexed `ui.nearest_canvas_object` metadata over rendered selectable canvas objects. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint181-ui-map-dirty-index-final-20260603-131018.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint181-ui-map-dirty-index-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 182 (UI Map Index Cache and Delta Watch)** is complete on `sprint-182-ui-map-index-cache`. It adds cached UI-node ID/role indexes, indexed `ui.index_stats`, `ui.get_node`, and `ui.nodes_by_role` agent lookups, bounded dirty-event history, and `ui.watch_delta` so agents can read semantic GUI changes without repeated full map dumps. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint182-ui-map-index-cache-final-20260603-133922.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint182-ui-map-index-cache-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 183 (Agent Protocol Catalog)** is complete on `sprint-183-agent-protocol-catalog`. It adds `agent.methods`, `agent.method_schema`, and `agent.quickstart` over the shared Agent-panel/live-socket dispatcher so agents can discover current GUI/project protocol methods, input-schema shapes, safety flags, dry-run support, and the recommended UI-map operating loop without scraping docs or guessing method names. Focused `gui_ui_map` and `gui_agent_panel` tests passed, official visual validation produced `artifacts/screenshots/sprint183-agent-protocol-catalog-final-20260603-140218.png`, the target harness proved semantic coordinates in `artifacts/screenshots/sprint183-agent-protocol-catalog-targets-final-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 184 (Library Placement Stability)** is complete on `sprint-184-library-placement-stability`. It adds persisted symbol snapshots for placed schematic components, schematic scene rendering from stored symbol primitives and pin leads, `ccad sch place-symbol`, concrete chooser row identity metadata, schematic-only GUI tab selection, and clean `ccad_gui --screenshot-project` visual capture. The full Qt build passed, CTest passed 36 of 36 tests, official visual validation produced `artifacts/screenshots/sprint184-library-placement-stability-shipping-20260603-152917.png`, target-sequence validation produced `artifacts/screenshots/sprint184-library-placement-stability-targets-shipping-target-sequence.json`, and schematic screenshot validation produced `artifacts/screenshots/sprint184-symbol-schematic-visual-shipping.png`.
- **Sprint 185 (KiCad Symbol Catalog Scale)** is complete on `sprint-185-kicad-symbol-catalog-scale`. It expands raw KiCad `.kicad_sym` libraries into top-level symbol chooser rows, carries `extends` metadata in chooser selections, uses the selected item name for lazy preview/final symbol loading, and adds core plus GUI regression tests. The full Qt build passed, CTest passed 36 of 36 tests, official visual validation produced `artifacts/screenshots/sprint185-kicad-symbol-catalog-scale-shipping-20260603-160243.png`, chooser visual validation produced `artifacts/screenshots/sprint185-kicad-symbol-chooser-shipping.png`, and target-sequence validation produced `artifacts/screenshots/sprint185-kicad-symbol-catalog-scale-targets-shipping-target-sequence.json`.
- **Sprint 186 (Agent Harness Foundation)** is complete on `sprint-186-agent-harness-foundation`. It moves the Agent panel to a right-side dock while preserving `panel:agent` and `tab:agent`, adds read-only GUI `agent.*` harness metadata for session state, run profile, safety, provider, observability, evidence manifest, and tool guides, and exposes matching headless CLI metadata commands plus JSON-RPC routes. Focused `gui_ui_map` and `agent_serve` tests passed, official visual validation produced `artifacts/screenshots/sprint186-agent-harness-foundation-rightsplit-20260604-150605.png`, target validation produced `artifacts/screenshots/sprint186-agent-harness-foundation-rightsplit-targets-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 187 (Agent Panel Workspace)** is complete on `sprint-187-agent-panel-workspace`. It upgrades the right-side Agent dock from a raw protocol console into a compact workspace surface with live project/epoch/view/layer/net/tool context, cached diagnostic counts, result-state summaries, one-click Context, Diagnostics, Tool Guide, and Clear actions, and stable UI-map targets for those controls. Focused `gui_agent_panel` and `gui_ui_map` coverage passed, the broader GUI focused gate passed 14 of 14 tests, official visual validation produced `artifacts/screenshots/sprint187-agent-panel-workspace-corrected-20260604-195903.png`, target validation produced `artifacts/screenshots/sprint187-agent-panel-workspace-targets-target-sequence.json`, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 188 (Agent Task Workspace)** is complete on `sprint-188-agent-task-workspace`. It adds a staged task-goal input, visible task state, bounded pinned evidence queue, `agent.workspace_state`, and semantic UI-map controls for goal typing, staging, evidence pinning, and evidence clearing. Focused `gui_agent_panel` and `gui_ui_map` coverage passed, official visual validation produced `artifacts/screenshots/sprint188-agent-task-workspace-corrected-20260604-203103.png`, target validation produced `artifacts/screenshots/sprint188-agent-task-workspace-targets-target-sequence.json` with the new Agent controls found before and after resize, and the full build plus CTest gate passed 36 of 36 tests.
- **Sprint 189 (Agent Approval Workspace)** is complete on `sprint-189-agent-approval-workspace`. It adds the first local approval lane to the right-side Agent dock with a targetable approval request input, Request, Accept, Decline, Cancel, Clear, visible approval status, and approval fields in `agent.workspace_state`. Focused `gui_agent_panel` and `gui_ui_map` coverage passed, official visual validation produced `artifacts/screenshots/sprint189-agent-approval-workspace-final-20260604-210030.png`, target validation produced `artifacts/screenshots/sprint189-agent-approval-workspace-targets-target-sequence.json` with every approval target found before and after resize, and the full build plus CTest gate passed 36 of 36 tests.

## Phase Roadmap

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, pads, vias, tracks, keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. Native GUI/editor: schematic and PCB review/edit surfaces, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, SPICE/simulation hooks, manufacturing exports.
6. Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, benchmark harness.

## Recently Completed

Sprint 65 completed the Phase 2 hardening batch for board validity and authoring safety. It added stricter ERC identity checks, DRC checks for board outline, net/member integrity, layer identity, physical object identity, pad logical parity, full-geometry board-edge checks, full-geometry keepout checks, and CLI authoring guards for edge overhangs and cross-type physical ID reuse.

Sprint 66 has added physical placement regions as first-class board primitives. They now round-trip through deterministic JSON, can be authored through `ccad pcb add-placement-region`, appear in inspect/review counts, are checked by DRC for identity, kind, size, board bounds, and physical ID reuse, and are visible in the Qt canvas/object browser/demo path.

Sprint 67 has started with board layer authoring. The first target is `ccad pcb add-layer`, giving agents a deterministic way to add layers after board initialization instead of editing JSON manually.

Sprint 67 completed board layer authoring through `ccad pcb add-layer`, including duplicate ID checks, optional visibility parsing, CLI tests, README usage, and demo coverage.

Sprint 68 has started board-level DRC rule configuration. The target is serialized design rules plus `ccad pcb set-rules` so copper clearance, minimum track width, and minimum via annular ring are no longer hardcoded behavior.

Sprint 68 completed board-level DRC rule configuration. Design rules now serialize with the board, `ccad pcb set-rules` updates them deterministically, and DRC obeys configured copper clearance, minimum track width, and via annular ring values.

Sprint 69 has started board outline mutation. The target is `ccad pcb set-outline` with guards that reject outlines which would leave existing board primitives or regions outside the board.

Sprint 69 completed board outline authoring through `ccad pcb set-outline`, including object-containment guards, CLI tests, README usage, and demo coverage.

Sprint 70 has started layer-aware visibility. The target is to make layer `visible` data affect review rendering, expose `ccad pcb set-layer-visibility`, and keep the object/layer browser clear about hidden layers.

Sprint 70 completed layer-aware visibility. CanvasScene now carries layer visibility, the Qt canvas hides pads and tracks on hidden layers while keeping vias visible, the object browser labels layer visibility, `ccad pcb set-layer-visibility` toggles existing layers, and the demo script exercises the command.

## Active Sprint

Sprint 71 has started origin-aware canvas rendering. The target is to make the GUI-independent canvas scene preserve board outline origin and make the Qt renderer draw pads, tracks, vias, keepouts, and placement regions relative to that origin.

Sprint 71 completed origin-aware canvas rendering. CanvasScene carries board outline origin, the Qt renderer draws board primitives and regions relative to that origin, tests verify origin metadata and rendered item coordinates, and the demo uses a non-zero-origin outline.

Sprint 72 has started layer-aware DRC hardening. The target is to keep copper clearance and connectivity checks faithful to physical layer semantics: pads and tracks connect/collide only on shared copper layers, while vias remain cross-layer copper.

Sprint 72 completed layer-aware DRC hardening. Track-to-pad endpoint contact now requires a shared copper layer, different-layer pads/tracks can overlap without clearance diagnostics, and vias still check clearance across layers.

Sprint 73 has started copper-layer authoring guards. The target is to reject pads, tracks, and placed footprint pads on non-copper layers through both DRC diagnostics and CLI mutation guards.

Sprint 73 completed copper-layer authoring guards. DRC now reports pads and tracks on non-copper layers, and the CLI rejects add-pad, add-track, and place-footprint requests that target non-copper layers.

Sprint 74 has started board-origin review reporting. The target is for ProjectReview and `ccad inspect` to report the full board outline rectangle, including origin and size, so agents can reason about shifted board coordinates without opening raw project JSON.

Sprint 74 completed board-origin review reporting. ProjectReview now carries board origin coordinates, and `ccad inspect` emits `x_nm`, `y_nm`, `width_nm`, and `height_nm` in board metadata.

Sprint 75 completed GUI board-origin summary display. The Qt project summary now shows non-zero board outline origin next to board size, keeps zero-origin boards compact, wraps long summary title/subtitle text inside the dock, and has a dedicated CTest panel target.

Sprint 76 completed review/inspect design-rule reporting. ProjectReview and `ccad inspect` now expose active board-level physical DRC rules, including copper clearance, minimum track width, and minimum via annular ring, so agents can reason from inspect JSON instead of raw project JSON.

Sprint 77 completed GUI design-rule summary display. The Qt project summary now shows active copper clearance, minimum track width, and minimum via annular ring values from ProjectReview near the top of the dock, so human review sees the same physical-rule context that agents get from inspect JSON.

Sprint 78 completed origin-aware cursor status. The Qt status bar coordinate readout now classifies Board versus Canvas positions using the actual board outline origin and max point, matching the origin-aware renderer and summary.

Sprint 79 completed design-rule DRC validation. DRC now reports non-positive copper clearance, minimum track width, and minimum via annular ring values when they arrive from hand-edited or imported project JSON.

Sprint 80 completed configured clearance diagnostics. `COPPER_CLEARANCE` messages now name the active board-level clearance value, so agents and humans can interpret findings without separately looking up the rule.

Sprint 81 completed rule-threshold diagnostic hardening. Minimum track-width and via annular-ring diagnostics now name the active configured threshold, matching the copper-clearance diagnostic behavior.

Sprint 82 completed via-ring diagnostic priority. DRC avoids reporting a derived annular-ring violation when the via drill already exceeds the via diameter or the via size is otherwise invalid.

Sprint 83 completed keepout diagnostic priority. DRC avoids reporting derived keepout geometry violations when the checked pad, via, or track has invalid dimensions.

Sprint 84 completed clearance diagnostic priority. DRC avoids reporting derived copper-clearance violations when the checked pad, via, or track has invalid dimensions.

Sprint 85 completed diagnostic JSON summaries. `validate` and `drc` include total, error, and warning counts next to the diagnostics array, so agents can read status without recounting the list.

Sprint 86 completed catalog diagnostic summaries. `lib catalog-validate` uses the same summary shape as `validate` and `drc`, keeping agent-facing diagnostic JSON consistent across project and catalog checks.

Sprint 87 completed catalog search summaries. `lib catalog-search` includes summary metadata with match counts while preserving its existing output fields.

Sprint 88 completed board-layer project diffs. `ccad diff` and transaction diff JSON report board layer additions, removals, and changes instead of only logical components, nets, and constraints.

Sprint 89 completed board-outline project diffs. `ccad diff` and transaction diff JSON report board outline additions, removals, and changes.

Sprint 90 completed board-pad project diffs. `ccad diff` and transaction diff JSON report board pad additions, removals, and changes.

Sprint 91 completed board route-primitive project diffs. `ccad diff` and transaction diff JSON now report board via and track additions, removals, and changes.

Sprint 140 completed Agent MCP Support.

Sprint 141 completed KiCad Component Ingestion and Rendering. It successfully ingested KiCad footprints and symbols, parsed their graphic primitives (Lines, Arcs, Circles, Polygons, Text), and natively rendered them in the CCad Qt Canvas, fully verified by automated screenshot pipelines.

Sprint 142 completed BOM Export. It emitted a CSV Bill of Materials from the `Project` model, providing a basic manufacturing output containing designators and parts.

Sprint 143 completed Pick and Place (PnP) Export. It emitted a CSV file containing component centroids and rotations from the PCB layout, facilitating board assembly.

Sprint 144 completed Drill Export. It emitted an Excellon NC Drill file from the PCB layout's vias and through-hole pads, enabling bare-board fabrication.

Sprint 92 completed board region project diffs. `ccad diff` and transaction diff JSON now report board keepout and placement-region additions, removals, and changes.

Sprint 93 completed board design-rule project diffs. `ccad diff` and transaction diff JSON now report board-level DRC rule changes.

Sprint 94 completed CLI board-diff coverage. The `ccad diff` executable test now verifies board-level physical entries for design rules, layers, placement regions, keepouts, pads, vias, and tracks.

Sprint 95 completed physical object removal authoring. `ccad pcb remove-object --file <path> --id <id>` removes pads, vias, tracks, keepouts, and placement regions by stable ID.

Sprint 96 completed board layer removal authoring. `ccad pcb remove-layer --file <path> --id <id>` removes unused board layers and rejects missing or referenced layers.

Sprint 97 completed physical object movement authoring. `ccad pcb move-object --file <path> --id <id> --x-mm <n> --y-mm <n>` moves pads, vias, keepouts, and placement regions with board-boundary guards.

Sprint 98 completed physical object resizing authoring. `ccad pcb resize-object --file <path> --id <id> --width-mm <n> --height-mm <n>` resizes pads, keepouts, and placement regions with board-boundary guards.

Sprint 99 completed track geometry editing. `ccad pcb set-track --file <path> --id <id> --start-x-mm <n> --start-y-mm <n> --end-x-mm <n> --end-y-mm <n> --width-mm <n>` updates existing track endpoints and width with board-boundary guards.

Sprint 100 completed via geometry editing. `ccad pcb set-via --file <path> --id <id> --diameter-mm <n> --drill-mm <n>` updates existing via diameter and drill with drill and board-boundary guards.

Sprint 101 completed pad metadata and rotation editing. `ccad pcb set-pad --file <path> --id <id> --component <id> --pin <name> --net <id> --layer <id> --rotation-deg <n>` updates existing pad binding, layer, and rotation with copper-layer and board-boundary guards.

Sprint 102 completed region kind editing. `ccad pcb set-region-kind --file <path> --id <id> --kind <kind>` updates existing keepout and placement-region kind values by stable ID.

Sprint 103 has started layer metadata editing. The target is `ccad pcb set-layer --file <path> --id <id> --name <name> --kind <kind> --visible true|false`, with guards that prevent referenced copper layers from being changed into non-copper metadata.

Sprint 103 completed layer metadata editing. `ccad pcb set-layer --file <path> --id <id> --name <name> --kind <kind> --visible true|false` updates existing layer name, kind, and visibility while rejecting referenced non-copper kind changes.

Sprint 104 has started route primitive metadata editing. The target is to extend `ccad pcb set-via` with optional net updates and `ccad pcb set-track` with optional net and copper-layer updates, without breaking existing geometry-only command usage.

Sprint 104 completed route primitive metadata editing. `ccad pcb set-via` accepts optional `--net`, and `ccad pcb set-track` accepts optional `--net` and `--layer`, with copper-layer validation for track layer changes.

Sprint 105 has started compact PCB object lookup. The target is `ccad pcb get-object --file <path> --id <id>`, emitting one board layer, pad, via, track, keepout, or placement region as JSON so agents can inspect stable objects without rereading the whole project file.

Sprint 105 completed compact PCB object lookup. `ccad pcb get-object --file <path> --id <id>` emits one board layer, pad, via, track, keepout, or placement region as JSON and rejects missing IDs.

Sprint 106 has started compact PCB object listing. The target is `ccad pcb list-objects --file <path> [--type <type>]`, emitting stable IDs and light metadata for board layers and physical objects without requiring agents to parse the full project file.

Sprint 106 completed compact PCB object listing. `ccad pcb list-objects --file <path> [--type <type>]` emits stable IDs and light metadata for board layers and physical objects, with optional type filters.

Sprint 107 has started compact PCB net listing. The target is `ccad pcb list-nets --file <path>`, emitting physical net usage counts for pads, vias, and tracks so agents can reason about board connectivity without parsing full project JSON.

Sprint 107 completed compact PCB net listing. `ccad pcb list-nets --file <path>` emits physical net usage counts for pads, vias, and tracks, ignoring empty net IDs.

Sprint 108 has started a CLI module split. The target is to move PCB object query JSON helpers out of `pcb_commands.cpp` into `pcb_object_queries.*` without changing behavior.

Sprint 108 completed the CLI module split. PCB object and net query JSON helpers now live in `src/ccad_cli/pcb_object_queries.hpp/.cpp`, keeping `pcb_commands.cpp` focused on command dispatch and mutation flow.

Sprint 109 has started the Phase 2 completion gate. Phase 2 is being closed because the board model, early authoring commands, DRC, diffs, query commands, and GUI review hooks are present and verified. Phase 3 opens next with constrained routing assistance and external router boundary work.

Sprint 109 completed the Phase 2 completion gate. README now marks Phase 3 as the active phase, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 110 planning.

Sprint 110 has started route-request model groundwork. The target is a deterministic board-level route-request record that captures routing intent without generating trace geometry yet.

Sprint 110 completed route-request model groundwork. `ccad::Board` now preserves deterministic `route_requests` JSON records, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 111 planning.

Sprint 111 has started route-request DRC validation. The target is to reject malformed route intent before route commands or solver integration consume it.

Sprint 111 completed route-request DRC validation. Route requests now receive identity, net, layer, endpoint, and width diagnostics, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 112 planning.

Sprint 112 has started CLI route-request authoring. The target is `ccad pcb add-route-request`, creating route intent records by stable endpoint object IDs without generating tracks.

Sprint 112 completed CLI route-request authoring. `ccad pcb add-route-request` now appends deterministic route intent records, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 113 planning.

Sprint 113 has started compact route-request listing. The target is `ccad pcb list-route-requests`, giving agents route intent summaries without parsing full project JSON.

Sprint 113 completed compact route-request listing. `ccad pcb list-route-requests` now emits route intent summaries, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 114 planning.

Sprint 114 has started route-request project diffs. The target is to make `ccad diff` and transaction diff summaries expose route intent changes.

Sprint 114 completed route-request project diffs. `ccad diff` now reports route_request additions, changes, and removals, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 115 planning.

Sprint 115 has started route-request editing. The target is `ccad pcb set-route-request`, letting agents update routing intent by stable ID without deleting and recreating records.

Sprint 115 completed route-request editing. `ccad pcb set-route-request` now updates existing route intent records by stable ID, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 116 planning.

Sprint 116 has started route-request removal. The target is `ccad pcb remove-route-request`, completing the add/list/update/remove lifecycle for route intent records before external router handoff work.

Sprint 116 completed route-request removal. `ccad pcb remove-route-request` now removes route intent records by stable ID, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 117 planning.

Sprint 117 has started external-router boundary groundwork. The target is `ccad pcb export-route-job`, emitting deterministic route-job JSON that a later router process can consume without scraping full project JSON.

Sprint 117 completed external-router boundary groundwork. `ccad pcb export-route-job` now emits compact deterministic route-job JSON, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 118 planning.

Sprint 118 has started route-job obstacle export. The target is to include keepouts and placement regions in `ccad pcb export-route-job` so future routers see board constraints, not just copper objects.

Sprint 118 completed route-job obstacle export. `ccad pcb export-route-job` now includes keepouts and placement regions, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 119 planning.

Sprint 119 has started scoped route-job export. The target is `ccad pcb export-route-job --request-id <id>`, allowing agents to hand external routers one route request at a time while preserving the same board context envelope.

Sprint 119 completed scoped route-job export. `ccad pcb export-route-job --request-id <id>` now exports one selected request or rejects missing IDs, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 120 planning.

Sprint 120 has started route-job metadata hardening. The target is to make `ccad pcb export-route-job` self-describing with schema version and unit metadata for external router consumers.

Sprint 120 completed route-job metadata hardening. `ccad pcb export-route-job` now declares schema version and units, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 121 planning.

Sprint 121 has started route-result application groundwork. The target is `ccad pcb apply-route-segment`, a minimal external-router return path that converts one satisfied route request into a board track segment.

Sprint 121 completed route-result application groundwork. `ccad pcb apply-route-segment` now converts one satisfied route request into a board track segment, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 122 planning.

Sprint 122 has started multi-segment route application groundwork. The target is `ccad pcb apply-route-segment --complete false`, letting external route results append intermediate track segments while keeping the request open until the final segment.

Sprint 122 completed multi-segment route application groundwork. `ccad pcb apply-route-segment --complete false` now leaves route requests open for later segments, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 123 planning.

Sprint 123 has started route-segment default-layer handling. The target is for `ccad pcb apply-route-segment` to use the route request's preferred layer when the external result does not provide an explicit layer.

Sprint 123 completed route-segment default-layer handling. `ccad pcb apply-route-segment` now defaults to the request preferred layer when `--layer` is omitted, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 124 planning.

Sprint 124 has started route-segment provenance. The target is for tracks created from route requests to preserve `source_route_request_id` through project JSON and route-job exports.

Sprint 124 completed route-segment provenance. Tracks now preserve `source_route_request_id` through project JSON, route-job export, and diffs, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 125 planning.

Sprint 125 has started compact track provenance queries. The target is for `ccad pcb get-object` and `ccad pcb list-objects --type track` to expose `source_route_request_id` without requiring agents to parse full project JSON.

Sprint 125 completed compact track provenance queries. Track `get-object` and `list-objects` output now include `source_route_request_id`, and the post-merge Windows Qt build plus CTest gate passed before moving to Sprint 126 planning.

Sprint 126 completed Phase 3 routing assistance as a real batch rather than another tiny counter increment. The batch added route-status reporting, completed/partial/open route progress counts, stricter route-request DRC for endpoint net mismatch, same endpoint, empty policy, and too-narrow route width, multi-segment `pcb apply-route-polyline`, route-status test coverage, DRC coverage, README command documentation, and a clean scripted GUI demo that exports route-job JSON, applies a routed polyline, writes route-status JSON, waits through the beep-and-20-second screenshot harness, and shows a clean board with completed route provenance. Phase 3 is now complete because CCad has typed route intent, route-job export, route-result application, route provenance, route progress reporting, DRC validity checks, CLI tests, and visible demo evidence. The sprint-end gate passed with a clean 87-step Qt build and 21 of 21 CTest tests passing.

Sprint 127 completed the first Phase 4 native GUI/editor route-review batch. The batch adds route-request and route-progress fields to the shared review model, carries route intent and track route provenance through the GUI-independent canvas model, shows route-request counts and progress in the project summary, adds route-request rows and route provenance to the object browser, lets route rows highlight tracks created from that request, and exposes the same route progress through `ccad inspect`. The GUI screenshot harness produced `artifacts/screenshots/sprint127-gui-route-review-rebuilt-20260527-145138.png`, where routed tracks `RT.1` and `RT.2` show `route RR1` provenance. The full clean Qt build and CTest gate passed with 21 of 21 tests passing.

Sprint 128 completed the KiCad layer compatibility foundation. The batch used current KiCad and industry layer references before implementation, added a canonical KiCad named-layer registry, exposed idempotent `ccad pcb add-standard-layers`, updated the sprint demo into a clean bridge-rectifier board, and kept README/progress/sprint documentation in the same sprint. The GUI screenshot harness produced `artifacts/screenshots/sprint128-bridge-rectifier-layer-foundation-clean2-20260527-155104.png`, where the board shows 59 layers, a clean status, four diode bridge legs, AC/DC pads, routed positive and negative buses, and no diagnostics.

Sprint 129 completed layer review summaries. The batch kept the KiCad layer-display reference rule active and added review-model, inspect JSON, and GUI project-summary visibility for layer categories and layer visibility state. The GUI screenshot harness produced `artifacts/screenshots/sprint129-layer-review-summary-visible-20260527-160424.png`, where the bridge board stays clean and the project summary shows the layer breakdown card. Inspect JSON reports `layer_summary` with 32 copper layers, 27 non-copper layers, 9 visible layers, and 50 hidden layers.

Sprint 130 completed interactive layer visibility. The right-dock layer browser now renders checkable list items for each layer. Signal-blocking prevents recursion during list population, and an `itemChanged` callback updates the in-memory board model in `ReviewWindow` and immediately triggers canvas re-rendering. The GUI screenshot harness produced `artifacts/screenshots/sprint-130-demo-20260530-225412.png` showing interactive checkable layer rows. The clean build and CTest gate passed with all 22 tests passing.

Sprint 131 completed coordinate inspection helpers in the Selection Inspector Panel to display unit-converted physical properties (position, dimension, net, layer, rotation, drill, track length, etc.) for selected PCB objects (pads, vias, tracks, keepouts, placement regions) based on the kernel-mediated Board model. It displays all physical lengths in both millimeters (mm) and mils (mil). The GUI screenshot harness produced `artifacts/screenshots/sprint-demo-20260530-233238.png` demonstrating coordinate inspection. The clean build and CTest gate passed with all 22 tests passing.

Sprint 132 completed interactive physical DRC design rule and object property editing in the Selection Inspector Panel, with callback updates to mutate the `ccad::Board` model, project file auto-save, view refreshes, and selection state preservation. The GUI screenshot harness produced `artifacts/screenshots/sprint-demo-20260530-235551.png` showing interactive inspector row fields. The clean build and CTest gate passed with all 22 tests passing.

Sprint 133 completed the KiCad S-expression PCB export serialization engine (`.kicad_pcb` format) in the CCad core kernel, exposed it via the `ccad pcb export-kicad` subcommand in the CLI, added help metadata, and verified formatting parity and CLI behavior through comprehensive unit and integration tests. The clean build and CTest gate passed with all 23 tests passing.

Sprint 134 completed KiCad Footprint S-expression export (`.kicad_mod`). The batch added `exportKiCadFootprint` serialization, floating point coordinate translation, and `ccad lib export-footprint` CLI command. The clean build and CTest gate passed with all 24 tests passing.

Sprint 135 completed KiCad/Specctra DSN Export functionality and `ccad pcb export-dsn`. The clean build and CTest gate passed with all 25 tests passing.

Sprint 136 completed Agent JSON-RPC Protocol foundation. It introduced `ccad agent serve` that runs a JSON-RPC 2.0 loop on stdin/stdout, routing commands and returning execution status and redirected stdout/stderr.

Sprint 137 completed Agent Audit Logs. `ccad_cli::writeProjectFile` now seamlessly captures `Transaction` records and appends them to `<project>.audit.jsonl` whenever the CLI mutates the board.

Sprint 138 completed Agent Permission Gates. The JSON-RPC `agent serve` now enforces explicit `--allow-read` and `--allow-write` boundaries before executing commands, returning standard JSON-RPC error `-32604` for unauthorized calls.

Sprint 139 completed Agent Benchmark Harness. Introduced `scripts/benchmark.py` which runs a specified agent command against a directory of `.ccad.json` files and executes `ccad drc` to validate correctness of the agent's work.

Sprint 140 completed Agent MCP Support and closed Phase 6. The `agent serve` loop now natively speaks the Model Context Protocol (MCP) by handling `initialize`, `tools/list`, and `tools/call`, allowing seamless integration with modern LLM-driven tooling.

Sprint 147 completed Interactive Authoring. It introduced `ccad_core/placement.hpp` for core footprint placement logic, replacing inline CLI logic. It also added `FootprintPlacementDialog` to the GUI, allowing users to interactively place footprints on the board canvas through the 'Add Footprint' toolbar action.

## Reporting Rule


Every commit/merge status update should include:

```text
Progress: Phase X/6, Sprint N, <branch-or-main>, <short status> <next sprint target> <expected sprint count to complete this phase>
```

## Sprint Integrity Rule

A sprint number is not a counter for tiny isolated commits. A sprint is a real scoped batch with a maintained sprint file under `docs/devops/sprints/`, and that file must describe the sprint goal, branch, tasks, bugs or risks, documentation updates, verification commands, verification results, and closure status. Do not start or close Sprint N unless the Sprint N file exists.

Documentation changes belong to the same sprint as the behavior they describe. README updates, progress counter updates, feature inventory updates, codebase-map or handover updates, and sprint notes must be completed before the sprint is closed. They must not be pushed into a later cleanup sprint unless the user explicitly approves that exception.

Sprint closure requires focused tests during development and the full configured build plus CTest gate at the end. GUI-visible work also needs a visible artifact or app-owned screenshot unless the user explicitly blocks visual capture for that run. Record the exact verification command and result in the sprint file before marking the sprint verified.

Each CCad sprint must deliver a moderate product batch, not a tiny one-commit increment. The minimum target is 8 to 10 visible or user-meaningful features, fixes, workflow improvements, or documented capabilities per sprint, unless the user explicitly approves a smaller safety sprint. A phase must also have a limited sprint budget before work starts, with the expected sprint count reported in progress updates, so a phase cannot continue indefinitely without a deliberate scope decision.

Because full CMake and CTest verification is heavy and token-consuming in this repository, do not run the full gate after every small implementation step. Use targeted checks only when they are cheap and necessary during development. At sprint end, before committing completion work or shipping the sprint, run the full configured build and CTest gate, fix any failures, rerun the failing or full gate as appropriate, then ship only after the gate passes.

## External Reference Rule

Every CCad feature or bugfix sprint must start with current external references for the feature domain before code edits begin. For KiCad-compatible PCB/CAD behavior, official KiCad documentation and KiCad developer file-format documentation are mandatory. Manufacturing, layer-stack, DRC, and fabrication work should add Altium or IPC-style industry references when relevant. Simulation work should add KiCad ngspice and ngspice references. AI harness and observability work should add OpenTelemetry, Langfuse, LangGraph, or equivalent primary references. The sprint file must include a "References Checked" section that records the sources, the behavior they imply, CCad's compatibility decision, and the verification evidence tied to that behavior. If internet lookup fails, record the failure and use local cached docs only as a temporary fallback.

Sprint 148 is verified on branch `sprint-148-gui-kicad-parity`. The batch adds KiCad-style icon toolbars sourced from the local KiCad checkout, shape-aware pad and drill rendering for annular-ring visibility, core-owned interactive placement and movement, GUI symbol and footprint placement entry points, legacy pad `layer_id` JSON compatibility, and the official visual screenshot proof. The sprint file is `docs/devops/sprints/2026-06-01-sprint-148-gui-kicad-parity.md`. The final clean rebuild passed all 177 build steps, and CTest passed 33 of 33 tests.

Sprint 154 is complete on branch `sprint-154-gui-actions-library-cache`. The batch replaced visible GUI top-toolbar placeholders with real Save, Board Setup, Undo, Redo, Run DRC, and DRC export behavior; made the local library chooser more KiCad-like with filter, library/name rows, detail metadata, and preview notes; resolved converted symbol `extends` inheritance from sibling `library-cache` JSON so derived symbols inherit pins before placement; and accepted raw KiCad `.kicad_mod` files from the footprint chooser.

Sprint 155 is complete and merged to `main`. The batch delivered KiCad-style cache-backed symbol and footprint choosing, tab-aware Add behavior, cursor-following placement ghosts with Escape cancellation, distinct top/bottom copper colors, full-width track selection highlighting, and a consolidated backlog for GUI, KiCad compatibility, library, simulation, manufacturing, LLM, and visual-validation work. The sprint-end full clean Qt build passed all 177 build steps and CTest passed 33 of 33 tests. The official visual harness produced `artifacts/screenshots/sprint155-kicad-placement-chooser-final-20260601-211645.png`.

Sprint 156 is complete on branch `sprint-156-lazy-library-chooser`. Add Symbol and Add Footprint now open from a cheap cache catalogue, parse only the selected item for detail metadata and a visual preview, and preserve the existing final placement import path. The GUI now has app-owned chooser screenshot modes and a live mouse/keyboard harness for placement-dialog checks. Footprint previews share the actual board canvas layer palette so PCB layer intent colors remain distinct. The sprint-end full clean Qt build passed all 177 build steps and CTest passed 33 of 33 tests. The official visual harness produced `artifacts/screenshots/sprint156-lazy-library-chooser-preview-final-20260602-004023.png`. The live mouse/keyboard harness produced `artifacts/screenshots/sprint156-live-footprint-preview-focus-20260602-004533.png` and `artifacts/screenshots/sprint156-live-symbol-preview-20260602-004647.png`, confirming the chooser previews through real GUI interaction. The layer-color rerun produced `artifacts/screenshots/sprint156-app-footprint-chooser-layer-colors.png` and `artifacts/screenshots/sprint156-app-symbol-chooser-layer-colors.png`.

Sprint 227 is complete on branch `sprint-227-fix-board-shape`. The batch addressed user feedback by maintaining the optimized $O(N \log N)$ closest-point spatial search complexity via a KD-tree powered by `nanoflann.hpp`. 


Sprint 228 is verified on branch `sprint-228-source-walk`. The batch progressed the KiCad source walk through `initpcb.cpp` to `padstack.cpp`, mapping GUI and data-model boundaries and stubbing advanced bridging/padstack functionality to the backlog.


Sprint 229 is verified on branch `sprint-229-source-walk`. The batch progressed the KiCad source walk through `pcb_barcode.cpp` to `project_pcb.cpp`, mapping GUI and rendering boundaries and stubbing advanced primitives (barcodes, dimensions, fields, groups, plotting, tables, text) to the backlog.


Sprint 230 is verified on branch `sprint-230-source-walk`. The batch concluded the KiCad source walk from Iteration 467, covering `sel_layer.cpp` through the end of the alphabet. Copper pours (zones) and track cleaning heuristics were stubbed to the backlog. Phase 8 is formally complete!


Sprint 231 is complete on branch `sprint-231-fix-ci`. Fixed the GitHub Actions CI/CD pipeline failure for `visual_harness_policy` by un-ignoring `.agents/workflows/*.md` in `.gitignore` so the required workflow file is tracked and available in the remote repository.


Sprint 232 is verified on branch `sprint-232-eeschema-source-walk`. The batch began the KiCad schematic source walk from `annotate.cpp` through `connection_graph.cpp`, mapping cross-probing and BOM logic to CCad native constructs, and stubbing hierarchical graph features to the backlog.

Sprint 232 continuation is verified on branch `sprint-232-schematic-model`. The batch repaired schematic symbol snapshot persistence after the model rename by storing embedded symbol snapshots on placed `SchSymbol` records, preserving legacy `components`/`symbols`, `part`/`lib_id`, and pin `kind`/`type` compatibility, expanding symbol snapshots into schematic canvas primitives, and updating the tabbed object-browser test. The incremental Qt build passed, focused placement, CLI, and object-browser tests passed, the full CTest gate passed 56 of 56 tests, the official visual harness produced `artifacts\screenshots\sprint232-symbol-snapshot-proof-internal-20260629-181543.png`, and the targeted schematic screenshot `artifacts\screenshots\sprint232-schematic-symbol-snapshot.png` visually proved reloaded symbol body and pin-lead geometry with empty stderr.


Sprint 233 is verified on branch `sprint-233-eeschema-source-walk`. The batch progressed the KiCad schematic source walk through `eeschema.cpp` to `junction_helpers.cpp`, omitting redundant wxWidgets GUI forms, serialization handlers, and stubbing schematic DXF/SVG imports to the backlog.


Sprint 234 is verified on branch `sprint-234-eeschema-source-walk`. The batch concluded the KiCad schematic source walk in `eeschema`, omitting redundant wxWidgets GUI forms, rendering engines, and stubbing core schematic primitives and hierarchical nets to the backlog.


Sprint 235 is verified on branch `sprint-235-gerbview-source-walk`. The batch began the KiCad Gerber Viewer source walk from `am_param.cpp` through `gbr_layout.cpp`, omitting redundant wxWidgets GUI forms and stubbing Gerber aperture and Excellon drill parsing to the backlog.


Sprint 236 is verified on branch `sprint-236-gerbview-source-walk`. The batch concluded the KiCad Gerber Viewer source walk in `gerbview`, omitting redundant wxWidgets GUI forms, and stubbing Gerber core graphic primitives, document models, and syntax parsers to the backlog.


Sprint 237 is verified on branch `sprint-237-3d-viewer-source-walk`. The batch concluded the KiCad 3D Viewer source walk in `3d-viewer`, omitting redundant wxWidgets GUI forms, and stubbing 3D geometry caching, rendering graphs, and transformations to the backlog.


Sprint 238 is verified on branch `sprint-238-cvpcb-source-walk`. The batch concluded the KiCad Footprint Assignment source walk in `cvpcb`, omitting redundant wxWidgets GUI forms, and stubbing schematic-to-PCB bridging and auto-association heuristics to the backlog.


Sprint 239 is verified on branch `sprint-239-router-source-walk`. The batch concluded the KiCad Autorouter and Interactive Router source walk in `pcbnew/autorouter` and `pcbnew/router`, omitting redundant wxWidgets GUI forms, and stubbing Push-and-Shove (PNS) algorithms and auto-placers to the backlog.


Sprint 240 is verified on branch `sprint-240-importer-source-walk`. The batch concluded the KiCad External EDA Formats source walk in `pcbnew/pcb_io`, omitting native KiCad format IO, and stubbing third-party format importers (Allegro, Altium, Eagle, etc.) to the backlog.


Sprint 241 is verified on branch `sprint-241-auxiliary-source-walk`. The batch concluded the KiCad auxiliary tools source walk in `pagelayout_editor`, `pcb_calculator`, and `bitmap2component`, omitting redundant wxWidgets GUI forms, and stubbing page layouts, RF calculators, and bitmap conversions to the backlog.



- **Sprint 227 (KiCad Additonal PCB Object Docs)** is complete. Updated the docs to reflect the completed source walk for pcb_barcode.cpp, pcb_dimension.cpp, pcb_group.cpp, pcb_reference_image.cpp, pcb_table.cpp, pcb_target.cpp, pcb_text.cpp. Fixed coordinate multiplier bug in CanvasReferenceImage and CanvasTable.

Sprint 242 is verified on branch `sprint-242-graphics-cleaner`. The batch completed the port of `graphics_cleaner.cpp` from KiCad, implementing the core algorithms for detecting and removing null shapes and duplicate geometries, as well as merging collinear redundant rectangles/lines into larger rectangles. The CLI was extended with `ccad pcb clean-graphics` and manual JSON serialization to respect CCad architecture. The build succeeded and the full CTest gate passed all 61 tests. The official GUI visual validation script confirmed no regressions, generating `artifacts/screenshots/sprint-demo-20260702-080715.png`.

Sprint 243 is verified on branch `sprint-243-schematic-annotation`. The batch ported KiCad's `annotate.cpp` automatic reference designator annotation logic to CCad native constructs in `annotate.hpp` and `annotate.cpp`. The implementation includes tracking used prefixes, spatial sorting via bounding box thresholds (Sort X and Sort Y), and applying algorithms to retain or reset existing designations. The feature was exposed natively via the `ccad sch annotate` CLI command. Isolated tests in `test_annotate.cpp` were incorporated into the CMake build, bringing the passing CTest suite up to 62 tests. The official sprint demo was invoked to visually prove system stability, outputting to `artifacts/screenshots/sprint-demo-20260702-082538.png`.

Sprint 244 is verified on branch `sprint-244-schematic-autoplace-fields`. The batch ported KiCad's `autoplace_fields.cpp` heuristic property layout logic to CCad native constructs in `autoplace_fields.hpp` and `autoplace_fields.cpp`. The implementation includes collision avoidance against nearby wires, and uses a simplified density algorithm based on nested `SymbolPin` locations to pick the clearest side (Top, Bottom, Left, or Right) for symbol property texts. The feature is exposed via the `ccad sch autoplace` CLI command. The robust `ccad_autoplace_fields_tests` were integrated to push the passing test count to 63/63. The visual sprint demo executed cleanly, proving stability, outputting to `artifacts/screenshots/sprint-demo-20260702-083917.png`.
## Sprint 358 progress update (2026-09-16)

The DRC provider-parity batch now validates through-hole pad drill presence and minimum size, pad copper edge clearance, and drilled via hole spacing. Full Qt build passed 70/70 and CTest passed 69/69; official visual proof `sprint358-hole-proof` passed with an inspected screenshot and empty harness stderr. The demo fixture did not exercise the new violation cases, which remain covered by focused DRC tests. Next ordered survey batch is scratch 058.
## Sprint 359 progress update (2026-09-16)

Removed the explicit FastMath3D sine/cosine stubs and added regression coverage. Corrected full Qt build passed 94/94, CTest passed 70/70, and the official visual proof passed with an inspected screenshot and empty stderr. KiCad survey continuation remains at scratch 058/059.
## Sprint 360 progress update (2026-09-16)

Implemented the Math3D 4x4 homogeneous transform and regression coverage. Full Qt build passed 67/67 incremental steps, CTest passed 70/70, and official visual proof passed with inspected screenshot and empty stderr. Continue KiCad survey at scratch 058-060.
## Sprint 361 progress update (2026-09-16)

Implemented deterministic Prim MST generation for core ratnest lines and added a four-node regression test. Full Qt build passed 94/94, CTest passed 71/71, and official visual proof passed with inspected screenshot and empty stderr. GUI ratsnest rendering remains a documented next integration step.
## Sprint 362 progress update (2026-09-16)

Implemented DynamicRatnestGraph board population for net-bearing pads, vias, and track endpoints. Full Qt build passed 95/95, CTest passed 72/72, and official visual proof passed with inspected screenshot and empty stderr. GUI ratsnest overlay remains the next integration step.
## Sprint 363 progress update (2026-09-16)

Connected the existing GUI ratsnest overlay to the core deterministic MST algorithm. Full incremental Qt build passed 9/9, CTest passed 72/72, and official visual proof passed with an inspected MST-style overlay screenshot and empty stderr. Next is feature-specific UI-map proof and connected-component semantics.
## Sprint 364 progress update (2026-09-16)

Completed event-driven ratnest recomputation: board modification now publishes per-net MST edges instead of clearing the active list. Full Qt build passed 69/69, CTest passed 72/72, and official visual proof passed with inspected screenshot and empty stderr.
## Sprint 364 UI-map proof update

Official UI-map mouse harness completed two 15-target passes for ratsnest toggle coverage; 22 generated screenshots loaded successfully, and the show-ratsnest target was exercised in both passes. Existing unavailable menu/tab/chat-input targets remain documented.
## Sprint 365 progress update (2026-09-16)

Repaired UI-map menu mnemonic, Agent dock, QTextEdit composer, and send-button target coverage. Official UI-map harness reached 15/15 targets in both passes with 30 PNGs ingested; final build passed 7/7, CTest 72/72, and official visual proof passed with inspected screenshot and empty stderr.
## Sprint 367 progress update (2026-09-16)

Added optional max track-width and via-diameter DRC rules across typed model, JSON, CLI, and diagnostics. Final rebuild passed 200/200, CTest 72/72, and official visual proof passed after resolving a stale GUI binary; screenshot inspected, stderr empty.
## Sprint 368 progress update (2026-09-16)

Made max track-width and max via-diameter rules discoverable in `pcb set-rules` help and added explicit CLI/JSON round-trip assertions. Targeted tests passed 3/3, full CTest passed 72/72, and official visual proof `sprint368-max-rule-help-proof` exited 0 with inspected screenshot and empty stderr. Ordered KiCad survey resumes at scratch 058.
## Sprint 369 progress update (2026-09-16)

Replaced track-length measurement stub with net-scoped Euclidean segment summation and added CTest `track_length_tuning`. Full build passed 96/96, CTest passed 73/73, and official visual proof `sprint369-track-length-proof` passed with inspected screenshot and empty stderr. Meander mutation remains explicitly deferred pending routing transaction semantics.
## Sprint 370 progress update (2026-09-16)

Extended net length measurement to three-point track arcs with circular sweep and collinear fallback. Focused test, full build 70/70, CTest 73/73, and official visual proof `sprint370-track-arc-length-proof` all passed; screenshot inspected, stderr empty.
## Sprint 371 progress update (2026-09-16)

Added board-thickness contribution for matching vias when height-for-length calculation is enabled, with toggle regression coverage. Full build 70/70, CTest 73/73, and official visual proof `sprint371-via-height-proof` passed; screenshot inspected, stderr empty.
## Sprint 372 progress update (2026-09-16)

Added `SILK_CLEARANCE` DRC for front/back silkscreen text against copper pads. Full build 70/70, CTest 73/73, and official visual proof `sprint372-silk-clearance-proof` passed; screenshot inspected and stderr empty. Rotation-aware text and other silk targets remain backlog.
## Sprint 373 progress update (2026-09-16)

Extended `SILK_CLEARANCE` to copper vias with regression coverage. Full build 70/70, CTest 73/73, and official visual proof `sprint373-silk-via-proof` passed; screenshot inspected, stderr empty.
## Sprint 374 progress update (2026-09-16)

Extended `SILK_CLEARANCE` to copper track segments with width-aware distance and regression coverage. Full build 70/70, CTest 73/73, and official visual proof `sprint374-silk-track-proof` passed; screenshot inspected, stderr empty.
## Sprint 375 progress update (2026-09-16)

Extended `SILK_CLEARANCE` to board-edge distance using text bounding-box corners. Full build 70/70, CTest 73/73, and official visual proof `sprint375-silk-edge-proof` passed; screenshot inspected, stderr empty.
## Sprint 376 progress update (2026-09-16)

Extended `SILK_CLEARANCE` to copper zones with polygon overlap/proximity detection. Full build 70/70, CTest 73/73, and official visual proof `sprint376-silk-zone-proof` passed; screenshot inspected, stderr empty.
## Sprint 377 progress update (2026-09-16)

Revalidated the complete silk clearance chain after zone integration: focused DRC 1/1, full build 70/70, CTest 73/73, and official visual proof `sprint376-silk-zone-proof` passed; screenshot inspected, stderr empty. Schematic parity is next; footprint/reference semantics remain under analysis.
## Sprint 379 progress update (2026-09-16)

Added schematic/board footprint parity diagnostics for missing, extra, and duplicate footprint references. Pad component IDs count as physical footprint presence when metadata is absent. Full build 70/70, CTest 73/73, and official visual proof `sprint379-parity-proof` passed; screenshot inspected, stderr empty, demo false missing reports removed.
## Sprint 380 progress update (2026-09-16)

Added `FOOTPRINT_BOM_PARITY`, comparing schematic `in_bom` with board footprint `exclude_from_bom`. Full build 70/70, CTest 73/73, and official visual proof `sprint380-bom-parity-proof` passed; screenshot inspected, stderr empty, demo parity remains clean.
## Sprint 381 progress update (2026-09-16)

Added explicit-footprint schematic pin to board pad parity with `MISSING_PAD`, matching component reference and pin number/name. Full build 70/70, CTest 73/73, and official visual proof `sprint381-pin-parity-proof` passed; screenshot inspected, stderr empty, demo parity remains clean.
## Sprint 382 progress update (2026-09-16)

Corrected parity for schematic-only symbols: `on_board=false` no longer requires footprint/pad presence. Focused DRC, full build 70/70, CTest 73/73, and official visual proof `sprint382-on-board-parity-proof` passed; screenshot inspected, stderr empty.
## Sprint 383 progress update (2026-09-16)

Added `SOLDERMASK_BRIDGE` DRC for expanded copper-pad mask webs between different nets, with focused regression coverage. Full build passed 70/70, CTest 73/73, and official visual proof `sprint383_soldermask_proof` exited 0; screenshot inspected, DRC summary recorded, stderr empty. Geometry is intentionally axis-aligned/global-expansion approximation; NPTH and per-pad mask overrides remain backlog.
## Sprint 384 progress update (2026-09-16)

Added typed minimum board-text height rule, JSON/CLI persistence, validation, and `TEXT_HEIGHT_BELOW_MINIMUM` DRC with regression coverage. Full build passed 212/212, CTest 73/73, and official visual proof `sprint384_text_height_proof` exited 0; screenshot inspected, stderr empty. Text thickness awaits explicit stroke-width model.
## Sprint 385 progress update (2026-09-16)

Added explicit board-text mirroring state, JSON persistence, CLI support, and front/back layer DRC diagnostics. Full build passed 200/200, CTest 73/73, and official visual proof `sprint385_text_mirror_proof` exited 0; screenshot inspected, stderr empty.
## Sprint 386 progress update (2026-09-16)

Added optional minimum/maximum connected-track angle rules with JSON/CLI persistence, validation, and `TRACK_ANGLE` DRC coverage. Full build passed 202/202, CTest 73/73, and official visual proof `sprint386_track_angle_proof` exited 0; screenshot inspected, stderr empty.
## Sprint 387 progress update (2026-09-16)

Added optional minimum/maximum straight track-segment length rules with JSON/CLI persistence, validation, and `TRACK_SEGMENT_LENGTH` DRC coverage. Full build passed 202/202, CTest 73/73, and official visual proof `sprint387_segment_length_proof` exited 0; screenshot inspected, stderr empty. Track-arc DRC remains next.
## Sprint 388 progress update (2026-09-16)

Extended `TRACK_SEGMENT_LENGTH` to three-point track arcs with circular-sweep and collinear fallback measurement. Full build passed 70/70, CTest 73/73, and official visual proof `sprint388_arc_length_proof` exited 0; screenshot inspected, stderr empty.
## Sprint 389 progress update (2026-09-16)

Added explicit board-text stroke width, minimum-thickness rule, JSON/CLI persistence, validation, and TEXT_THICKNESS_BELOW_MINIMUM DRC. Full build 200/200, CTest 73/73, official visual proof passed; screenshot inspected, stderr empty.
## Sprint 390-391 progress update (2026-09-16)

Implemented the clearance DRC provider pad/pad, pad/via, and via/via checks with focused coverage. Repaired CI portability: POSIX CLI test now preserves literal `${VAR}` tokens, MSVC forward declarations match struct definitions, duplicate math macro definitions are removed, π is guarded locally, and nanometer-to-floating conversions are explicit. Full Qt build 112/112, CTest 75/75, focused CLI and clearance tests passed; official visual proof `sprint391_ci_portability_proof` passed with empty stderr and inspected screenshot.
## Sprint 392 progress update (2026-09-16)

Implemented `DrcTestProviderEdgeClearance` for pad and via copper against rectangular board edges, with nanometer-safe distance conversion and focused regression coverage. Full Qt build 99/99, CTest 76/76, official visual harness `sprint392_edge_clearance_proof` passed; before/after screenshots individually inspected, stderr empty. Provider output remains kernel diagnostic; exact diagnostic rendering remains tied to main DRC pipeline.
## Sprint 393 progress update (2026-09-16)

Wired via edge-clearance reporting into primary `runDrc`, producing `VIA_EDGE_CLEARANCE` for copper reaching configured board-edge clearance. Focused DRC/provider tests passed; rebuilt CLI/GUI before targeted proof. Near-edge `TD_VIA` produced two errors including `VIA_EDGE_CLEARANCE`; target screenshot inspected, stderr empty.
## Sprint 394 progress update (2026-09-16)

Implemented `DrcTestProviderUnrouted` using deterministic physical-node components and exact track-endpoint unions. Disconnected same-net pads/vias now report error code 2; multi-segment connections resolve as connected. Focused test passed; full Qt build 100/100, CTest 77/77, official harness `sprint394_unrouted_proof` before/after screenshots individually inspected, stderr empty.

## Sprint 395 progress update (2026-09-16)

Integrated deterministic unrouted physical-net checking into primary `runDrc`. Same-net pad/via endpoints are grouped with exact-coordinate track endpoints; disconnected physical components now emit `UNROUTED_NET` errors consumed by CLI review and the Qt diagnostics/marker path. Added regression coverage in `tests/test_drc.cpp`. Full Qt build completed 74/74 and Qt-path CTest completed 77/77. Official harness `sprint395_unrouted_primary_proof` ran on rebuilt binaries; DRC artifact contains `UNROUTED_NET` for `DC_NEG` and `DC_POS`, both screenshots were visually inspected, stdout/stderr showed no GUI crash or warning. UI-map lookup confirmed `action:run_drc` and `panel:diagnostics`; safe-trigger correctly refused direct DRC execution because it requires human/kernel approval.

## Sprint 396 progress update (2026-09-16)

Replaced `NetTieDrc` placeholder behavior with deterministic component-backed tie semantics. Registered ties require both declared nets on the component; intersection checks use their axis-aligned pad span; missing-net ties are reported. Focused test passes; full gate and feature-specific visual proof remain.

## Sprint 397 progress update (2026-09-16)

Replaced `RouterTool` interactive no-op with deterministic kernel routing state. Start/update/commit/cancel and `routeTrack` now create or discard typed track segments. Focused test passed; Qt build 102/102, Qt-path CTest 79/79, official harness `sprint397_router_tool_proof` passed, screenshot inspected.

## Sprint 399 progress update (2026-09-16)

Added active-net endpoint snapping to `RouterTool`: pad/via targets within 0.75 mm are selected during route updates, and committed segments retain the active net. GUI semantic routing sets the active net before invoking the kernel gesture. Focused snap test passed; full rebuild 81/81, Qt-path CTest 79/79, live route harness 42/42, persisted 42-track inspection, and routed-board screenshot verification passed.

## Sprint 400 progress update (2026-09-16)

Added two-segment Manhattan routing for diagonal `RouterTool::routeTrack` requests; axis-aligned requests remain one segment. GUI `ui.route_track` now uses this complete kernel path. Full rebuild 81/81, Qt-path CTest 79/79, official harness passed, and live GUI harness confirmed 42/42 calls produced 84 persisted tracks. Final screenshot inspected.

## Sprint 401 progress update (2026-09-16)

Added clearance-aware route rejection against different-net pads using board copper clearance plus route half-width. Same-net routes remain allowed; blocked routes leave board track count unchanged. Focused test passed; full rebuild 76/76, Qt-path CTest 79/79, official harness and final routed screenshot passed inspection.

## Sprint 398 progress update (2026-09-16)

Wired GUI semantic `ui.route_track` automation through shared `RouterTool` kernel state. Active PCB layer and net are applied to the committed segment, then the project is saved and re-rendered. Focused GUI tests passed; live agent route harness performed 42/42 routes and persisted 42 tracks; rebuilt routed-board screenshot inspected with clean stdout/stderr.
## Sprint 402 progress update (2026-09-16)

Added RouterTool rejection for same-layer, different-net track obstacles using exact segment intersection plus copper-clearance endpoint checks. Fixed collinear non-overlap handling, prevented diagonal routes from committing a second segment after a blocked first segment, and passed active F.Cu/B.Cu through GUI semantic routing. Focused router test passed; Qt Release build completed 81/81; Qt-path CTest completed 79/79; official visual harness produced and screenshot inspection confirmed board canvas, layers, routed geometry, and agent panel remain rendered without crash.
## Sprint 403 progress update (2026-09-16)

Extended RouterTool copper-obstacle rejection to different-net vias. Via diameter contributes conservative radial clearance and blocks both layers because the current model has no via layer-span field. Focused router test, full Qt build, full Qt-path CTest, official harness, and screenshot inspection passed. Track arcs and zones remain next obstacle classes.
## Sprint 404 progress update (2026-09-16)

RouterTool now rejects candidate segments entering or approaching filled different-net zones on the selected copper layer. Polygon edge intersection, endpoint clearance, and interior checks use the existing physical geometry model. Focused router test passed; Qt Release build completed 76/76; Qt-path CTest completed 79/79; official visual harness screenshot was inspected. Zone checks remain conservative and do not yet model filled-island holes or thermal connections.
## Sprint 405 progress update (2026-09-16)

RouterTool now conservatively rejects different-net `TrackArc` crossings and vertex-clearance violations on the active copper layer by testing the arc's two chord segments and three defining points. Focused router test passed; Qt Release build completed 76/76; Qt-path CTest completed 79/79; official visual harness screenshot was inspected. Exact circular arc tessellation remains future refinement.
## Sprint 406 progress update (2026-09-16)

Improved different-net TrackArc routing rejection from two-chord testing to a 16-sample quadratic envelope, catching curved-span crossings between defining vertices while retaining width and clearance checks. Qt Release build completed 76/76; Qt-path CTest completed 79/79; official visual harness screenshot was inspected. Exact circumcircle sweep and adaptive tessellation remain future precision work.
## Sprint 407 progress update (2026-09-16)

TrackArc route-obstacle checks now sample the true three-point circumcircle sweep containing the arc midpoint, with quadratic fallback for collinear points. This removes quadratic-envelope distortion while retaining 16-segment conservative collision sampling and width/clearance checks. Qt Release build completed 76/76; Qt-path CTest completed 79/79; official visual harness screenshot was inspected.
## Sprint 408 progress update (2026-09-16)

Replaced quadratic TrackArc obstacle approximation with circumcircle reconstruction and midpoint-containing sweep sampling. Collinear arcs retain fallback. Qt Release build completed 76/76; Qt-path CTest completed 79/79; official visual harness screenshot inspected. Analytic closest-point clearance remains a precision backlog item.
## Sprint 409 progress update (2026-09-16)

Agent-panel provider fallback messages now render as compact `noticeCard` warnings instead of oversized generic chat bubbles. Targeted agent-panel/UI-map tests passed; Qt Release build completed 87/87; Qt-path CTest completed 79/79; official visual harness screenshot was ingested and visually confirmed notice hierarchy, readable text, stable canvas, and intact local-tool status.
## Sprint 410 progress update (2026-09-16)

GUI semantic route responses now distinguish `blocked_obstacle` from `zero_length`, exposing RouterTool obstacle rejection to agents without mutating the board. Targeted router and agent-panel tests passed; Qt Release build completed 90/90; Qt-path CTest completed 79/79; official visual harness screenshot was inspected and showed stable canvas, layers, and agent panel.
## Sprint 411 progress update (2026-09-16)

Agent route responses now include obstacle class in reason values: `blocked_obstacle_pad`, `via`, `arc`, `zone`, or `track`. Targeted tests passed; Qt Release build completed 81/81; Qt-path CTest completed 79/79; official visual harness screenshot was ingested and inspected.
## Sprint 412 progress update (2026-09-16)

Different-net track obstacle checks now include existing track half-width in clearance, preventing overlap with wide copper even when centerlines do not cross. Qt Release build completed 84/84; Qt-path CTest completed 79/79; official visual harness screenshot was ingested and inspected.
## Sprint 413 progress update (2026-09-16)

Added dedicated `router_width_clearance` regression target proving a wide different-net track blocks a near-parallel route without relying on the monolithic router fixture. Qt Release build completed 318/318 graph steps; Qt-path CTest completed 80/80; official visual harness screenshot was ingested and inspected.
## Sprint 414 progress update (2026-09-16)

Persisted board-zone holes through native JSON serialization and made routes wholly contained within one hole pass through while boundary entry remains blocked. Fixed MSVC portability by explicitly casting TrackArc coordinates and matching `Project`'s struct forward declaration. Corrected track-clearance unit conversion from nanometres to millimetres. CI-equivalent core build completed 324/324 and CTest 64/64 with MinGW runtime PATH; Qt Release CTest completed 80/80. Official harness `sprint414_ci_zoneholes` produced screenshot and stdout/stderr logs; screenshot inspection confirmed stable PCB canvas, layer panel, agent panel, and routed geometry.
## Sprint 415 progress update (2026-09-16)

Made the CLI cross-probe regression shell-portable by escaping `$NET` on POSIX shells through the existing `shellLiteral` helper. Removed an orphaned tracked Copilot gitlink with no `.gitmodules` URL, eliminating checkout cleanup failure. Core CLI target build and focused CTest passed; official visual harness from Sprint 414 remains the unchanged GUI proof for this CLI-only sprint.
## Sprint 416 progress update (2026-09-16)

Added typed front/back courtyard polygon storage to `BoardFootprint`, native JSON round-trip coverage, and a first courtyard DRC provider slice that reports deterministic front/back polygon overlaps. Existing missing/malformed courtyard and PTH/NPTH-specific rules remain deferred until import semantics are defined. Qt build completed 239/239; CTest completed 81/81; focused courtyard test passed; official visual harness screenshot was ingested and inspected with empty stderr.

## Sprint 419 progress update (2026-09-16)

Added a dedicated `CanvasFootprint` identity record and board-scene emission for footprint reference, value, layer, position, and rotation. The renderer now exposes a selectable footprint marker with canonical reference identity. Find-by-reference action wiring remains the next slice. Qt Release build completed 337/337; CTest completed 81/81; official visual harness screenshot was ingested and inspected with empty stderr.

## Sprint 420 progress update (2026-09-16)

Edit > Find now resolves case-insensitive footprint references against typed canvas identities, selects the matching footprint, refreshes selection state, and reports clear not-found status. Qt Release build completed 130/130; isolated CLI CTest passed; full Qt CTest completed 81/81 after rerunning transient agent-orchestrator hang; official harness completed and screenshot was visually inspected with empty stderr. Remote run 35084020399 remains older-SHA failure: exposed CLI cross-probe assertion is not reproducible on current main.

## Sprint 421 progress update (2026-09-16)

SES import now propagates enclosing Specctra `(net ...)` identity onto imported track segments and vias. This preserves routability metadata instead of silently producing geometry with empty nets. Focused DSN test passed; full Qt CTest completed 81/81; official visual harness completed with screenshot inspection and empty stderr. Via dimensions remain defaulted because SES references external padstack definitions, recorded as future parser work.

## Sprint 423 progress update (2026-09-16)

SES importer now resolves via diameter from route-library padstack circle shapes, matching KiCad Specctra import behavior; net identity remains preserved. Drill stays at safe fallback when SES padstack IDs carry no encoded drill. Focused DSN test and full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 424 progress update (2026-09-16)

SES via import now parses KiCad-style padstack IDs such as `Via_15:8_mil`, converting encoded drill size to CCad nanometres while retaining the safe fallback for generic IDs. Focused DSN test and full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 425 progress update (2026-09-16)

Added typed via start/end layer IDs, JSON persistence, and SES inference from padstack circle layer extrema. This gives blind/buried via imports a representable layer span while preserving through-via behavior for F.Cu/B.Cu definitions. Focused DSN test and full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 426 progress update (2026-09-16)

SES via padstack validation now rejects a referenced padstack without a supported circle shape, matching KiCad’s explicit import error; missing external padstacks retain compatibility fallback. Focused DSN test and full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 427 progress update (2026-09-16)

Via layer-span fields now carry default initializers and follow existing aggregate fields, preventing `-Werror=missing-field-initializers` regressions in older callers and GUI construction sites. Retry build completed 316/316; full Qt CTest passed 81/81; final official harness completed and screenshot was ingested and visually inspected with empty stderr.

## Sprint 428 progress update (2026-09-16)

Added explicit `Via::via_type` persistence with SES inference for through, blind, buried, and microvia classes from layer span and drill size. Legacy vias default to through. Build completed 88-target focused rebuild; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 429 progress update (2026-09-16)

CLI object queries now expose via type and layer-span metadata, allowing agents and scripts to distinguish imported via classes without reopening project JSON. CLI CTest passed; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 430 progress update (2026-09-16)

Via selection inspector now displays Via Type, Start Layer, and End Layer beside editable diameter/drill values. Targeted GUI inspector test passed; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 431 progress update (2026-09-16)

DRC now validates non-through via type and layer spans, reporting invalid identical endpoints, unknown layers, and unsupported via types. Targeted DRC test passed; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 432 progress update (2026-09-16)

Excellon drill export now preserves non-through via type and layer span as standards-safe comment records while leaving tool definitions and drill coordinates unchanged. Drill export test passed; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 433 progress update (2026-09-16)

Excellon export now rejects non-through vias with incomplete or identical layer spans before writing manufacturing output, while retaining metadata comments for valid spans. Drill export test passed; full Qt CTest passed 81/81; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 434 progress update (2026-09-16)

`JobManager::waitAll()` now waits for both queued and actively running tasks through a completion condition variable instead of spinning only until the queue empties. Added a delayed-task regression test. Qt Release build completed 166/166; full Qt CTest passed 82/82; official harness screenshot was ingested and visually inspected with empty stderr. CI run 35105302796 passed.

## Sprint 435 progress update (2026-09-16)

JobManager workers now catch task exceptions, report failures, decrement active-task state, and continue processing later tasks. Regression test confirms a throwing task does not kill worker service. Qt Release build completed 79/79; full Qt CTest passed 82/82; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 436 progress update (2026-09-16)

Implemented `AgentRunner::load_queue()` for the native queue schema. It restores pending goal/task identity, context, and tool arguments, rejects malformed roots/trailing data, and supports save/load restart continuity. Focused round-trip and malformed-input tests passed; Qt Release build completed 79/79; full Qt CTest passed 82/82; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 437 progress update (2026-09-16)

AgentPanel session checkpoints now persist the local run-queue state, and session loading restores queue ID, status, current step, counts, depth, and cancelability through the existing JSON session path. GUI panel test passed; changed Qt targets rebuilt 28/28; full Qt CTest passed 82/82; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 438 progress update (2026-09-16)

Added feature-specific GUI regression coverage for run-queue checkpoint round-trip: a session checkpoint writes `run_queue_state`, and a second AgentPanel restores the visible queue depth. Focused GUI test passed; full Qt CTest passed 82/82; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 439 progress update (2026-09-16)

Replaced `convertImageToPolygons()` bounding-box placeholder with alpha-aware, color-preserving horizontal span polygons scaled in nanometres. Added raster conversion regression coverage. Qt build completed 106/106; full Qt CTest passed 83/83; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 440 progress update (2026-09-16)

Replaced empty `convertSVGToLibShapes()` stub with headless parsing for common SVG line, rectangle, polygon, and polyline elements, including fill, offset, and pixel-scale conversion. Focused SVG test passed; Qt build completed 80/80; full Qt CTest passed 83/83; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 441 progress update (2026-09-16)

`DiffPairTuning::calculateCurrentSkew()` now measures routed TrackSegment lengths for positive/negative nets and returns absolute skew in millimetres, with safe zero result for missing pairs. Focused diff-pair test passed; Qt build completed 107/107; full Qt CTest passed 84/84; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 442 progress update (2026-09-16)

Hardened unsupported `DiffPairTuning::applyTuning()` to return `false` instead of claiming a successful no-op mutation; real meander topology remains explicitly deferred. Focused regression passed; Qt build completed 81/81; full Qt CTest passed 84/84; official harness screenshot was ingested and visually inspected with empty stderr.

## Sprint 443 progress update (2026-09-16)

Schematic labels now support explicit Directive type across the model, KiCad-style collector, and JSON serialization while preserving legacy global boolean compatibility. Focused tests passed; Qt Release build completed 231/231; full Qt CTest passed 84/84; official harness passed with empty stderr, and its screenshot was ingested and visually inspected.

## Sprint 444 progress update (2026-09-16)

Replaced the board reference-image crossed-box renderer placeholder with base64 image decoding and Qt pixmap rendering, retaining an explicit fallback marker for invalid image data. Focused GUI test passed; Qt Release build completed 18/18 incremental targets; full Qt CTest passed 84/84; official harness passed with empty stderr, and its screenshot was ingested and visually inspected.

## Sprint 445 progress update (2026-09-16)

Added optional KiCad-compatible sheet-pin side serialization and side-aware canvas orientation while preserving legacy position inference. Hardened the CLI integration test with a unique per-process temporary workspace, eliminating concurrent-run file races. Full Qt build completed; Qt-correct CTest passed 84/84, isolated CLI passed, official harness passed with empty stderr, and its screenshot was ingested and visually inspected.

Added serialization regression coverage proving a sheet-pin `side` value round-trips through JSON; incremental rebuild and full Qt CTest remained green at 84/84.

## Sprint 446 progress update (2026-09-16)

Replaced the empty PNS spatial-query placeholder with a radius-aware integer index. `PnsItem` now carries position and radius, `PnsNode` maintains index membership across add/clear, and duplicate index entries are ignored. Added `pns_index` regression coverage for hit radius, deduplication, misses, and invalid radius. Qt-correct full CTest passed 85/85; official harness passed with empty stderr, and its screenshot was ingested and visually inspected.

## Sprint 447 progress update (2026-09-16)

Made the PNS differential-pair placer perform minimal coupled placement instead of claiming placeholder success. It rejects identical endpoints, exposes a clamped gap setting, and places positive/negative items at stable configured separation during start and route. Focused regression passed; full gate and visual proof pending.

## Sprint 448 progress update (2026-09-16)

Replaced the PNS meander placer no-op with observable polyline state. Start records and places origin, meander appends changed destinations while suppressing duplicates, and finish clears active state. Focused regression passed; full gate and visual proof pending.

## Sprint 450 progress update (2026-09-16)

Added measurable target progress to PNS meander placement. `length()`, `remainingLength()`, and `targetReached()` expose actual polyline progress with safe target clamping. Focused and full CTest passed; official visual proof pending.

## Sprint 451 progress update (2026-09-16)

Added `meanderToTarget()`: direct destination when already sufficient, otherwise one perpendicular bend is generated from target deficit, then destination is appended. Regression proves bend generation and target reach; full CTest passed, official visual proof pending.

## Sprint 453 progress update (2026-09-16)

Extended target meander generation with optional multi-bend count while retaining one-bend default compatibility. Alternating perpendicular bends now create a first serpentine-like path; clearance, obstacle avoidance, and optimal amplitude remain future work.

## Sprint 455 progress update (2026-09-16)

Added `PnsNode::removeItem()` with synchronized index removal and ownership erase. Regression proves removed items disappear from node queries and repeated removal is safe. Full build/CTest and official visual harness passed; screenshot inspected.

## Sprint 456 progress update (2026-09-16)

Added `PnsNode::hasObstacle()` as a direct clearance decision over indexed expanded-radius queries. Regression covers obstacle hit and distant miss. Full build/CTest and official harness passed; screenshot inspected.


## Sprint 457 progress update (2026-09-16)

Made `PnsNode::addItem()` ownership-deduplicating, matching existing index deduplication. Duplicate insertion no longer leaves hidden duplicate ownership after removal. Full build/CTest and official harness passed; screenshot inspected.

## Sprint 458 progress update (2026-09-16)

Added `PnsIndex::querySegment()` and node forwarding. Segment queries use point-to-segment distance plus item radius and clearance, enabling future router adapters to detect mid-segment obstacles. Focused and full gates passed; official harness screenshot inspected with empty stderr.

## Sprint 459 progress update (2026-09-17)

Added PNS item net/layer identity and filtered segment queries. Callers can exclude same-net items and restrict obstacles to one layer before routing adaptation. Focused regression passed; full gate and visual proof pending.

## Sprint 459 progress update (2026-09-17)

Added PNS item net/layer identity and filtered segment queries. Callers can exclude same-net items and restrict obstacles to one layer before routing adaptation. Full build/CTest and official harness passed; screenshot inspected with empty stderr.

## Sprint 461 progress update (2026-09-17)

Added `PnsBoardObstacleIndex::blockingItems()` so adapter callers can retrieve blocker identities, not only boolean status. This supports actionable agent/router diagnostics. Full build/CTest and official harness passed; screenshot inspected with empty stderr.

## Sprint 462 progress update (2026-09-17)

Integrated `PnsBoardObstacleIndex` into `RouterTool::commitRouting()` as a pad/via fallback after existing precise checks. New regression proves different-net pad blocking and diagnostic reason, while tracks/zones/arcs retain existing authoritative paths. Full build/CTest and official harness passed; screenshot inspected with empty stderr.

## Sprint 463 progress update (2026-09-17)

Filtered PNS via obstacles by declared blind/buried layer span, preserving through-via behavior and endpoint fallback for incomplete metadata. Added regression covering a blind via excluded from B.Cu. Focused test passed, full build 86/86, CTest 89/89, and official harness passed with inspected screenshot and zero stderr.

## Sprint 464 progress update (2026-09-17)

Added real PNS segment items with full endpoint geometry and segment-to-segment clearance checks. The board adapter now indexes different-net tracks on the active copper layer, with crossing-track regression coverage. Full build 96/96, CTest 89/89, and official harness passed; screenshot inspected and stderr empty.

## Sprint 465 progress update (2026-09-17)

Made Agent panel actions semantically targetable by assigning stable object IDs and tooltips to navigation, templates, settings, attach, marketplace, context refresh, voice, and send controls. Focused GUI test, full CTest 89/89, official harness, and screenshot inspection passed; stderr empty.

## Sprint 466 progress update (2026-09-17)

Added PNS arc obstacles using a typed arc item and a 16-step quadratic envelope with track-width clearance. Active-layer, different-net arcs now enter the board adapter; focused regression, full build 96/96, CTest 89/89, official harness, and screenshot inspection passed with empty stderr.

## Sprint 467 progress update (2026-09-17)

Fixed POSIX CLI cross-probe test quoting: packets containing `$NET` now use a platform-safe shell argument instead of relying on nested double-quote escaping. This directly addresses Linux CTest failure at `pcb cross-probe reports net packet kind`. Focused CLI test, full CTest 89/89, and official visual harness passed; stderr empty.

## Sprint 468 progress update (2026-09-17)

Hardened cross-probe packet normalization against an argv-preserved backslash before `$`, so shell transport cannot change `$NET` packet classification. Added core regression. Focused 2/2 tests, full build 85/85, CTest 89/89, and official harness passed; screenshot inspected and stderr empty.

## Sprint 469 progress update (2026-09-17)

Added typed PNS polygon obstacles for filled, active-layer, different-net zones. Segment checks detect polygon boundary crossings and routes whose endpoints lie inside the solid; zone holes remain explicitly deferred. Focused test, full build 96/96, CTest 89/89, official harness, and screenshot inspection passed with empty stderr.

## Sprint 470 progress update (2026-09-17)

Extended PNS polygon obstacles with zone holes. Routes strictly inside holes are excluded from solid-zone blocking, while contour boundaries remain conservative. Focused test, full build 96/96, CTest 89/89, official harness, and screenshot inspection passed with empty stderr.

## Sprint 472 progress update (2026-09-17)

Added `calculateZoneFill()` to the core. It produces a deterministic fill result from enabled zone outer and hole contours, computes net contour area, preserves contour identity, and rejects malformed holes with diagnostics. Clearance knockouts, thermal reliefs, island removal, and obstacle clipping remain explicitly deferred to later filler slices. Focused tests passed 2/2; full Qt Release build completed 118/118; CTest passed 91/91; official harness passed, screenshot inspected, stderr empty.

## Sprint 473 progress update (2026-09-17)

Exposed the first fill result through `ccad pcb refill-zones --file <path> [--zone-id <id>]`. The command loads a project without mutation, calculates deterministic contour results, and emits machine-readable JSON with fill state, contour count, area, and diagnostics. CLI regression passed; full Qt Release build completed 118/118; CTest passed 91/91; official harness passed, screenshot inspected, stderr empty.
## Sprint 471 progress update (2026-09-17)

Added finite, non-negative validation for global zone thermal spoke width, thermal gap, and minimum island area settings. Focused zone-settings test passed; Qt Release build completed 118/118; full CTest passed 90/90; official harness passed with screenshot inspection and empty stderr. KiCad `ZONE_FILLER` comparison confirms actual filled polygon storage, clearance knockouts, thermal reliefs, and island policy remain future implementation work.
