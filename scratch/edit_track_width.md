## edit_track_width
- **File**: `pcbnew/edit_track_width.cpp`
- **Purpose**: UI glue code for handling track and via width adjustments.
- **Functionality**: 
  - `SetTrackSegmentWidth()` modifies existing tracks or vias to match either DRC rule constraints, specific netclass overrides (like microvias), or current global user settings. It handles pushing the changed item onto the undo stack.
  - `Tracks_and_Vias_Size_Event()` processes drop-down or toolbar selections that set the *current* global track/via size indices (e.g., custom sizes vs. netclass defaults).
- **Context**: Connects older GUI event loops and dropdown boxes to the design settings state block.
