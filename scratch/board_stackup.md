## BOARD_STACKUP / BOARD_STACKUP_ITEM / DIELECTRIC_PRMS
- **File**: `pcbnew/board_stackup_manager/board_stackup.h`, `pcbnew/board_stackup_manager/board_stackup.cpp`
- **Purpose**: Physical board layer stackup — the ordered list of material layers (copper, dielectric, soldermask, silkscreen, paste) that make up the manufactured PCB.

### DIELECTRIC_PRMS
- Per-sublayer dielectric parameters: `m_Material` (material name), `m_Thickness` (IU), `m_ThicknessLocked`, `m_EpsilonR` (dielectric constant), `m_LossTangent`, `m_SpecFreq` (frequency at which εr is specified), `m_DielectricModel` (CONSTANT / frequency-corrected model), `m_Color`.

### BOARD_STACKUP_ITEM_TYPE enum
- `BS_ITEM_TYPE_COPPER`: Copper layer.
- `BS_ITEM_TYPE_DIELECTRIC`: Substrate between copper layers (FR4, Rogers, etc.).
- `BS_ITEM_TYPE_SOLDERPASTE`: Solder paste layer.
- `BS_ITEM_TYPE_SOLDERMASK`: Solder mask (also a specialized dielectric).
- `BS_ITEM_TYPE_SILKSCREEN`: Silkscreen layer.

### BOARD_STACKUP_ITEM
- Represents one physical layer in the stackup. Key fields: `m_Type`, `m_LayerId` (PCB_LAYER_ID, or UNDEFINED_LAYER for dielectrics), `m_DielectricLayerId` (1..31 for dielectrics), `m_DielectricPrmsList` (vector of `DIELECTRIC_PRMS` — one per sublayer, > 1 for complex microwave boards). Key: `GetSublayersCount()`, `AddDielectricPrms/RemoveDielectricPrms`, `GetLayerDistance` (height between copper layers for via length calculation).

### BOARD_STACKUP
- Extends `SERIALIZABLE` (protobuf). Owns `vector<BOARD_STACKUP_ITEM*>`.
- **Key Methods**:
  - `BuildDefaultStackupList(settings, copperLayerCount)`: Creates a default copper+dielectric stack from board settings.
  - `SynchronizeWithBoard(settings)`: Adds missing layers, removes disabled layers.
  - `BuildBoardThicknessFromStackup()`: Sums all layer thicknesses.
  - `GetLayerDistance(firstLayer, secondLayer)`: Physical distance (IU) between two copper layers — used for via-length computation in length-matched routing.
  - `FormatBoardStackup(formatter, board)`: Writes stackup to `.kicad_pcb` file.
  - `Serialize/Deserialize`: Protobuf round-trip for the IPC API.
- **Extra properties**: `m_FinishType`, `m_HasDielectricConstrains`, `m_HasThicknessConstrains`, `m_EdgeConnectorConstraints` (NONE/IN_USE/BEVELLED), `m_EdgePlating`.

### BS_EDGE_CONNECTOR_CONSTRAINTS enum
- NONE, IN_USE, BEVELLED — used in Gerber job file to specify board edge connector requirements.
