# Sprint 18 Design: Rectangular Keepouts

## Goal

Add rectangular board keepouts to the core model, JSON format, and DRC.

## Why

Phase 2 explicitly includes keepouts and early DRC. A keepout primitive gives agents a machine-readable way to mark forbidden board regions before placement/routing automation exists.

## Model

```cpp
struct Keepout {
  std::string id;
  std::string kind;
  Rect area;
};
```

`Board` owns `std::vector<Keepout> keepouts`.

## JSON

Board JSON gains:

```json
"keepouts": [
  {
    "id": "K1",
    "kind": "placement",
    "area": {
      "x_nm": 20000000,
      "y_nm": 20000000,
      "width_nm": 4000000,
      "height_nm": 3000000
    }
  }
]
```

## DRC

- Pad center inside keepout: `PAD_IN_KEEPOUT`.
- Via center inside keepout: `VIA_IN_KEEPOUT`.
- Track start or end inside keepout: `TRACK_ENDPOINT_IN_KEEPOUT`.

All are errors for now.

## Non-Goals

- Do not add polygon keepouts.
- Do not test full trace intersection through a keepout.
- Do not add CLI keepout authoring in this sprint.
- Do not render keepouts in the GUI in this sprint.

## Definition Of Done

- Serialization test round-trips keepouts.
- DRC test catches pad, via, and track endpoint keepout violations.
- Full native Qt build and CTest pass.
