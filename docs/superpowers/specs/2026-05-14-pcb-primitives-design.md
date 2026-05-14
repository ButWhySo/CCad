# PCB Drawable Primitives Design

## Purpose

Move CCad from board outline rendering toward actual PCB layout data. This sprint adds pads, vias, and track segments as source-of-truth kernel objects and renders them read-only in the GUI.

## Scope

In scope:

- `Pad`: id, component ID, pin name, net ID, position, size, layer ID.
- `Via`: id, net ID, position, diameter, drill.
- `TrackSegment`: id, net ID, layer ID, start, end, width.
- JSON serialization.
- Canvas scene conversion and Qt rendering.

Out of scope:

- Editing.
- DRC.
- Footprint library.
- Zones/arcs.
- Router.

## Design

All geometry remains integer nanometers. Canvas conversion uses millimeter view units. Primitives carry semantic IDs and net/layer references so later selection, DRC, and transactions can address them without pixels.

