# Sprint 17 Design: DRC Empty Via/Track Net Warnings

## Goal

Warn when vias or tracks have empty `net_id`, matching the existing unconnected-pad diagnostic style.

## Why

After unknown-net checks and track endpoint warnings, empty via/track nets remain silent. They are useful for early geometry experiments, so they should not be hard errors yet, but agents need typed warnings to distinguish intentional partial work from forgotten net assignment.

## Behavior

- Empty via `net_id`: warning `UNCONNECTED_VIA`.
- Empty track `net_id`: warning `UNCONNECTED_TRACK`.
- Existing empty pad behavior remains warning `UNCONNECTED_PAD`.
- Non-empty unknown net IDs remain errors.

## Non-Goals

- Do not make empty via/track nets errors yet.
- Do not require routing completeness yet.
- Do not change CLI mutation commands in this sprint.

## Definition Of Done

- Focused DRC tests cover empty via and track net warnings.
- Existing unknown-net errors remain green.
- Full native Qt build and CTest pass.
