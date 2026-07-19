## fp_tree_model_adapter
- **File**: `pcbnew/fp_tree_model_adapter.cpp`, `pcbnew/fp_tree_model_adapter.h`
- **Purpose**: Adapts footprint library data into a `wxDataViewModel` for use in `wxDataViewCtrl` UI components (like the footprint tree).
- **Functionality**: 
  - Subclasses `LIB_TREE_MODEL_ADAPTER`.
  - Takes a `FOOTPRINT_LIBRARY_ADAPTER` and populates the `LIB_TREE_NODE` hierarchy.
  - Groups footprints by their libraries and correctly assigns pinning data from user settings.
- **Context**: Used to construct the hierarchical data structure behind the footprint library browser/chooser trees.

## fp_tree_synchronizing_adapter
- **File**: `pcbnew/fp_tree_synchronizing_adapter.cpp`, `pcbnew/fp_tree_synchronizing_adapter.h`
- **Purpose**: A synchronizing version of the footprint tree model adapter, designed for the Footprint Editor where libraries might change dynamically on disk.
- **Functionality**: 
  - Extends `FP_TREE_MODEL_ADAPTER`.
  - Contains `Sync()` which refreshes the `FOOTPRINT_LIBRARY_ADAPTER` and adds/removes individual library nodes and footprints without tearing down the entire UI tree.
  - Provides rich formatting (`GetAttr`, `GetValue`) so the footprint editor can strikethrough or bold items that are currently active or modified on the canvas.
  - Implements the hover-preview panel functionality (`ShowPreview`).
- **Context**: Used specifically by `FOOTPRINT_TREE_PANE` in the footprint editor.
