## cross-probing
- **File**: `pcbnew/cross-probing.cpp`
- **Purpose**: Facilitates IPC communication (cross-probing) between the PCB Editor (Pcbnew) and Schematic Editor (Eeschema).
- **Functionality**: 
  - `ExecuteRemoteCommand()` parses simple socket string messages (`$NET:`, `$NETS:`, `$CLEAR`, `$CONFIG`) and triggers PCB UI actions (highlighting nets, clearing highlights, zooming to items).
  - `KiwayMailIn()` handles higher-level `KIWAY_MAIL_EVENT`s sent over Kiway express mail (e.g., `MAIL_PCB_GET_NETLIST`, `MAIL_PCB_UPDATE_LINKS`, `MAIL_SELECTION`, `MAIL_CROSS_PROBE`).
  - Converts cross-probe selections (`F<ref>`, `P<ref>/<pad>`) into physical board items and triggers the `syncSelection` UI action.
  - Generates component strings formatted as `$PART: "U1" $PAD: "2"` to send selection state back to Eeschema.
- **Context**: Critical for keeping the schematic and board selection and highlighting in sync. Relies on `kiway_mail.h` and the underlying `KIWAY` infrastructure for local routing.
