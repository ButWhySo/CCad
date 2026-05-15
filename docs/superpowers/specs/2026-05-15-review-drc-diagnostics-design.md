# Sprint 33 Design: Review DRC Diagnostics

## Purpose

The GUI diagnostic table can now select canvas objects by stable ID, but the review model still only carries ERC diagnostics. Sprint 33 makes the review stream include physical DRC diagnostics as well, so PCB violations such as pads in keepouts appear in the same review table and can use the selection link.

## Scope

`ccad_core::buildReview` should append `runDrc(project)` results after ERC diagnostics and count both sources when computing error and warning totals. The GUI and CLI inspect surfaces already consume `ProjectReview`, so they should receive the richer diagnostics without separate GUI parsing.

## Non-Goals

This sprint does not add graphical marker overlays, diagnostic auto-centering, diagnostic explanations, or repair commands.

## Verification

Use TDD by adding a review test that creates a physical DRC violation and expects the review diagnostics to include the DRC code and object ID. Run the full native Qt build and CTest gate before committing.
