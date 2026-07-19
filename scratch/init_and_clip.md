## initpcb
- **File**: `pcbnew/initpcb.cpp`
- **Purpose**: Initialization logic for creating empty / new PCB boards and footprints.
- **Functionality**: 
  - `PCB_EDIT_FRAME::Clear_Pcb()` and `FOOTPRINT_EDIT_FRAME::Clear_Pcb()` prompt for unsaved changes, clear the undo/redo stack, and initialize a new `BOARD` object.
  - Sets up the default layers, design settings, default netclasses, and DRC severities for the new workspace.
- **Context**: Used when the user creates a "New Board" or clears the current footprint editor session.

## invoke_pcb_dialog
- **File**: `pcbnew/invoke_pcb_dialog.h`
- **Purpose**: An isolation layer header to decouple frames from modal dialog implementations.
- **Functionality**: 
  - Currently exposes `InvokePcbLibTableEditor()`, which summons the modal dialog for managing global/project footprint library tables.
- **Context**: Reduces include dependencies across the codebase by not requiring every caller to include the full dialog header.

## kicad_clipboard
- **File**: `pcbnew/kicad_clipboard.cpp`, `pcbnew/kicad_clipboard.h`
- **Purpose**: Interfaces with the system clipboard (via wxWidgets) to serialize and deserialize selected KiCad items.
- **Functionality**: 
  - `CLIPBOARD_IO` inherits from `PCB_IO_KICAD_SEXPR` (the standard S-Expression reader/writer).
  - Uses a dummy `(kicad_pcb)` wrapper string when copying arbitrary selection sets to ensure the S-expr parser can properly re-hydrate the items on paste.
  - Specially handles nested tables, groups, pads (stripping nets), and text fields (evaluating variables on copy).
  - Uses `wxTheClipboard` to push or pull text data.
- **Context**: Backend for all Copy (Ctrl+C) and Paste (Ctrl+V) operations within PCB editor canvases.
