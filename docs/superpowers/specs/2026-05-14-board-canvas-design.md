# Board Canvas Design

## Purpose

Add the first visual PCB canvas to CCad. The GUI should begin moving toward a KiCad-like CAD workspace, but without creating GUI-only state or editing behavior.

## Scope

In scope:

- Core canvas view-model for board outline.
- Qt `QGraphicsView` rendering of board outline.
- Empty state when no board exists.
- No editing, selection, routing, or footprint rendering.

Out of scope:

- Panning/zoom controls beyond default view behavior.
- Tracks, pads, footprints, vias, zones.
- Layer rendering.
- DRC overlays.

## Architecture

`ccad_core/canvas.hpp` converts `Project` board data into `CanvasScene`. The GUI renders that scene. This keeps rendering inputs deterministic and testable before richer CAD drawing starts.

## Visual Direction

The canvas should feel like a modern CAD panel:

- dark board workspace
- visible board outline
- subtle grid/background
- review side panel retained
- no decorative hero-like UI

