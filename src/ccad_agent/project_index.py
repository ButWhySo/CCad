"""Revision-aware retrieval over the authoritative typed CCad project snapshot."""

from __future__ import annotations

import hashlib
import json
import math
import re
from collections import Counter, defaultdict
from typing import Any


_WORD = re.compile(r"[\w]+", re.UNICODE)
_SECRET = re.compile(
    r"(?:api[_-]?key|secret|password|token)\s*[:=]\s*\S+|"
    r"\b(?:sk|csk|gsk|xai|sk-or)-[A-Za-z0-9_-]{12,}\b|\bAIza[A-Za-z0-9_-]{20,}",
    re.IGNORECASE)
_COORDINATE = re.compile(
    r"(?<![\w.])(-?\d+(?:\.\d+)?)\s*(mm|cm|mil|mils|in|inch|inches)?\s*[,;]\s*"
    r"(-?\d+(?:\.\d+)?)\s*(mm|cm|mil|mils|in|inch|inches)?",
    re.IGNORECASE)
_RADIUS = re.compile(
    r"(?:within|radius(?:\s+of)?|near|around|close\s+to|beside)\s+"
    r"(\d+(?:\.\d+)?)\s*(mm|cm|mil|mils|in|inch|inches)?",
    re.IGNORECASE)
_STOP = {"the", "and", "for", "with", "from", "into", "near", "around", "show",
         "find", "what", "where", "this", "that", "there", "board", "schematic",
         "project", "objects", "please", "current", "existing", "all", "are", "is"}
_MAX_INPUT_ENTITIES = 50_000
_MAX_RESULT_TEXT = 400


def _normalize(value: Any) -> str:
    return " ".join(str(value or "").casefold().split())


def _safe(value: Any, limit: int = 120) -> str:
    text = str(value or "").strip()
    if not text or _SECRET.search(text):
        return ""
    return text[:limit]


def project_model(snapshot: Any) -> dict:
    """Unwrap only native project models; never interpret arbitrary context as a model."""
    if isinstance(snapshot, str):
        try:
            snapshot = json.loads(snapshot)
        except (TypeError, json.JSONDecodeError):
            return {}
    if not isinstance(snapshot, dict):
        return {}
    typed = snapshot.get("typed_state")
    if isinstance(typed, dict):
        if not typed.get("available", False):
            return {}
        project = typed.get("project")
        if not isinstance(project, dict):
            return {}
        diagnostics = snapshot.get("project_diagnostics")
        if isinstance(diagnostics, list):
            # The GUI attaches live diagnostics beside typed_state; carry them
            # into the index view without mutating the cached typed project.
            project = dict(project)
            project["project_diagnostics"] = diagnostics
        return project
    project = snapshot.get("project", snapshot)
    if not isinstance(project, dict):
        return {}
    if not any(key in project for key in ("board", "boards", "components", "symbols", "nets")):
        return {}
    return project


def _point(value: Any) -> tuple[float, float] | None:
    if not isinstance(value, dict):
        return None
    try:
        if "x_nm" in value and "y_nm" in value:
            x, y = float(value["x_nm"]) / 1_000_000, float(value["y_nm"]) / 1_000_000
        elif "x_mm" in value and "y_mm" in value:
            x, y = float(value["x_mm"]), float(value["y_mm"])
        elif "x" in value and "y" in value:
            x, y = float(value["x"]), float(value["y"])
        else:
            return None
    except (TypeError, ValueError, OverflowError):
        return None
    if not math.isfinite(x) or not math.isfinite(y) or max(abs(x), abs(y)) > 1_000_000:
        return None
    return x, y


def _points(value: Any) -> list[tuple[float, float]]:
    if isinstance(value, dict):
        if "position_x_nm" in value and "position_y_nm" in value:
            return _points({"x_nm": value["position_x_nm"],
                            "y_nm": value["position_y_nm"]})
        direct = _point(value)
        if direct is not None:
            return [direct]
        points = []
        # Board keepouts and placement regions serialize an axis-aligned
        # Rect as either {origin:{x_nm,y_nm},size:{width_nm,height_nm}} or
        # {x_nm,y_nm,width_nm,height_nm}. Keep geometry interpretation tied
        # to those explicit typed fields; do not guess from arbitrary JSON.
        area = value.get("area")
        if isinstance(area, dict):
            origin = _point(area.get("origin", area))
            size = area.get("size", area)
            if origin is not None and isinstance(size, dict):
                try:
                    divisor = 1_000_000 if any(
                        key in size for key in ("width_nm", "height_nm")) else 1.0
                    width = float(size.get("width_nm", size.get("width_mm", 0))) / divisor
                    height = float(size.get("height_nm", size.get("height_mm", 0))) / divisor
                    if all(math.isfinite(v) and abs(v) <= 1_000_000
                           for v in (width, height)):
                        x, y = origin
                        points.extend(((x, y), (x + width, y + height)))
                except (TypeError, ValueError, OverflowError):
                    pass
        if "x_mm" in value and "y_mm" in value and any(
                key in value for key in ("width_mm", "height_mm")):
            try:
                x, y = float(value["x_mm"]), float(value["y_mm"])
                width, height = float(value.get("width_mm", 0)), float(value.get("height_mm", 0))
                if all(math.isfinite(v) and abs(v) <= 1_000_000
                       for v in (x, y, width, height)):
                    points.extend(((x, y), (x + width, y + height)))
            except (TypeError, ValueError, OverflowError):
                pass
        for key in ("start", "mid", "end", "position", "text_position"):
            points.extend(_points(value.get(key)))
        for key in ("points", "outline", "outer", "filled_contours",
                    "front_courtyard", "back_courtyard"):
            points.extend(_points(value.get(key)))
        return points
    if isinstance(value, list):
        return [point for item in value for point in _points(item)]
    return []


def _bbox(points: list[tuple[float, float]]) -> dict | None:
    if not points:
        return None
    xs, ys = zip(*points)
    return {"min_x_mm": min(xs), "min_y_mm": min(ys),
            "max_x_mm": max(xs), "max_y_mm": max(ys)}


def _unit_to_mm(unit: str) -> float:
    return {"": 1.0, "mm": 1.0, "cm": 10.0, "mil": 0.0254,
            "mils": 0.0254, "in": 25.4, "inch": 25.4,
            "inches": 25.4}.get(unit.casefold(), 1.0)


def _coordinates(query: str) -> list[tuple[float, float]]:
    found = []
    for match in _COORDINATE.finditer(query):
        x_unit = match.group(2) or match.group(4) or ""
        y_unit = match.group(4) or match.group(2) or ""
        point = (float(match.group(1)) * _unit_to_mm(x_unit),
                 float(match.group(3)) * _unit_to_mm(y_unit))
        if max(abs(point[0]), abs(point[1])) <= 1_000_000:
            found.append(point)
        if len(found) == 4:
            break
    return found


def _radius(query: str) -> float:
    match = _RADIUS.search(query)
    if not match:
        return 10.0
    return min(10_000.0, max(0.01, float(match.group(1)) * _unit_to_mm(match.group(2) or "")))


def _iter_dicts(value: Any):
    if isinstance(value, list):
        for item in value[:_MAX_INPUT_ENTITIES]:
            if isinstance(item, dict):
                yield item


class ProjectIndex:
    """In-memory exact, BM25, relationship, and spatial indexes for one CCad project."""

    CELL_MM = 10.0

    def __init__(self, *, max_entities: int = 12, max_chars: int = 6000):
        self.max_entities = max(1, min(32, int(max_entities)))
        self.max_chars = max(512, min(16_000, int(max_chars)))
        self._project_id = ""
        self._revision = ""
        self._has_snapshot = False
        self._docs: dict[str, dict] = {}
        self._aliases: dict[str, set[str]] = defaultdict(set)
        self._alias_tokens: dict[str, set[str]] = defaultdict(set)
        self._postings: dict[str, dict[str, int]] = defaultdict(dict)
        self._doc_terms: dict[str, Counter] = {}
        self._doc_lengths: dict[str, int] = {}
        self._net_docs: dict[str, set[str]] = defaultdict(set)
        self._component_docs: dict[str, set[str]] = defaultdict(set)
        self._sheet_docs: dict[str, set[str]] = defaultdict(set)
        self._reference_docs: dict[str, set[tuple[str, str]]] = defaultdict(set)
        self._layer_docs: dict[str, set[str]] = defaultdict(set)
        self._spatial_cells: dict[tuple[int, int], set[str]] = defaultdict(set)
        self._spatial_keys: dict[str, tuple[tuple[int, int], ...]] = {}
        self._spatial_global: set[str] = set()

    @staticmethod
    def _signature(doc: dict) -> str:
        indexed = {"fields": doc["fields"], "aliases": sorted(doc["aliases"]),
                   "tokens": dict(doc["tokens"]), "net": doc["net"],
                   "components": sorted(doc["components"]),
                   "layers": sorted(doc["layers"]),
                   "sheets": sorted(doc["sheets"]),
                   "references": sorted(doc["references"]),
                   "text": doc["text"]}
        return hashlib.sha256(json.dumps(
            indexed, sort_keys=True, separators=(",", ":")).encode()).hexdigest()

    @staticmethod
    def _make_doc(kind: str, item: dict, *, id_key: str = "id",
                  aliases=(), position=None, points=None, net_id="", layer_id="",
                  component_id="", sheet_id="", parent_sheet_id="",
                  extra_text=()) -> dict | None:
        object_id = _safe(item.get(id_key), 120)
        if not object_id:
            object_id = next((_safe(item.get(key), 120) for key in aliases
                              if _safe(item.get(key), 120)), "")
        if not object_id:
            return None
        fields: dict[str, Any] = {"id": object_id, "kind": kind}
        for key, value in (("reference", item.get("reference")),
                           ("value", item.get("value")),
                           ("name", item.get("name")),
                           ("part", item.get("part")),
                           ("pin_name", item.get("pin_name")),
                           ("pin_number", item.get("pin_number")),
                           ("type", item.get("kind", item.get("type"))),
                           ("code", item.get("code")),
                           ("severity", item.get("severity")),
                           ("message", item.get("message")),
                           ("engine", item.get("engine")),
                           ("object_id", item.get("object_id"))):
            clean = _safe(value, 140)
            if clean:
                fields[key] = clean
        net = _safe(net_id or item.get("net_id"), 120)
        if not net and kind.endswith("net"):
            net = object_id
        layer = _safe(layer_id or item.get("layer_id") or item.get("layer"), 120)
        if not layer and kind == "layer":
            layer = object_id
        layer_values = [layer, _safe(item.get("preferred_layer_id"), 120)]
        for key in ("start_layer_id", "end_layer_id"):
            value = _safe(item.get(key), 120)
            if value:
                fields[key] = value
                layer_values.append(value)
        for key in ("layers", "layer_ids"):
            values = item.get(key)
            if isinstance(values, (list, tuple)):
                layer_values.extend(_safe(value, 120) for value in values[:32]
                                    if isinstance(value, str) and _safe(value, 120))
        padstack = item.get("padstack")
        if isinstance(padstack, dict):
            values = padstack.get("layer_set")
            if isinstance(values, (list, tuple)):
                layer_values.extend(_safe(value, 120) for value in values[:32]
                                    if isinstance(value, str) and _safe(value, 120))
        layer_ids, seen_layers = [], set()
        for value in layer_values:
            normalized = _normalize(value)
            if normalized and normalized not in seen_layers:
                seen_layers.add(normalized)
                layer_ids.append(value)
        component = _safe(component_id or item.get("component_id") or
                           (item.get("reference") if kind in
                            {"footprint", "schematic_symbol"} else ""), 120)
        component_values = {_normalize(value) for value in (
            component, object_id if kind == "schematic_symbol" else "",
            _safe(item.get("symbol_id"), 120)) if _normalize(value)}
        if net:
            fields["net_id"] = net
            if kind == "schematic_pin":
                fields["membership_kind"] = "schematic_net_member"
        if layer:
            fields["layer_id"] = layer
        if layer_ids:
            fields["layer_ids"] = layer_ids
        if component:
            fields["component_id"] = component
        symbol_id = _safe(item.get("symbol_id"), 120)
        if symbol_id:
            fields["symbol_id"] = symbol_id
        source_sheet = _safe(sheet_id or item.get("sheet_id"), 120)
        parent_sheet = _safe(parent_sheet_id or item.get("parent_sheet_id"), 120)
        if source_sheet:
            fields["sheet_id"] = source_sheet
        if parent_sheet:
            fields["parent_sheet_id"] = parent_sheet
        for key in ("anchor_pad_id", "anchor_via_id", "anchor_track_id",
                    "from_object_id", "to_object_id", "preferred_layer_id"):
            clean = _safe(item.get(key), 120)
            if clean:
                fields[key] = clean
        raw_position = position if position is not None else item.get("position")
        point = _point(raw_position)
        if point is None:
            point_list = points if points is not None else _points(item)
            point = point_list[0] if point_list else None
        if point is not None:
            fields["position_mm"] = {"x": round(point[0], 6), "y": round(point[1], 6)}
        bounds = _bbox(points if points is not None else _points(item))
        if bounds is None and point is not None:
            bounds = {"min_x_mm": point[0], "min_y_mm": point[1],
                      "max_x_mm": point[0], "max_y_mm": point[1]}
        if bounds:
            fields["bounds_mm"] = {key: round(value, 6) for key, value in bounds.items()}
        searchable_values = [object_id, net, component]
        searchable_values.extend(_safe(item.get(key), 140) for key in aliases
                                 if key not in {"start_layer_id", "end_layer_id"})
        searchable_values.extend(fields.get(key, "") for key in
                                 ("reference", "value", "name", "part", "pin_name", "pin_number"))
        exact_values = [object_id]
        exact_values.extend(_safe(item.get(key), 140) for key in aliases
                            if key in {"id", "reference", "name", "pin_name", "pin_number"}
                            or (kind == "project_diagnostic" and
                                key in {"code", "object_id"}))
        if kind in {"layer", "schematic_net"}:
            exact_values.append(net or _safe(item.get("name"), 140))
        text = " ".join([kind] + [str(value) for value in searchable_values if value] +
                        [_safe(value, 160) for value in extra_text if _safe(value, 160)])
        tokens = Counter(token.casefold() for token in _WORD.findall(text)
                         if len(token) <= 80)
        doc = {"uid": f"{kind}:{object_id}", "fields": fields,
                "aliases": {_normalize(value) for value in exact_values if _normalize(value)},
                "tokens": tokens, "net": net.casefold(),
                "component": component.casefold(), "components": component_values,
                "layers": {_normalize(value) for value in layer_ids
                           if _normalize(value)},
                "sheets": {_normalize(source_sheet)} if _normalize(source_sheet) else set(),
                "references": set(),
                "text": text[:1200]}
        for key, relation in (("from_object_id", "route_endpoint"),
                              ("to_object_id", "route_endpoint"),
                              ("anchor_pad_id", "anchored_to"),
                              ("anchor_via_id", "anchored_to"),
                              ("anchor_track_id", "anchored_to"),
                              ("object_id", "diagnostic_for"),
                              ("parent_sheet_id", "child_sheet")):
            target_value = parent_sheet if key == "parent_sheet_id" else item.get(key)
            target = _normalize(target_value)
            if target:
                doc["references"].add((target, relation))
        members = item.get("members")
        if isinstance(members, list):
            for member in members[:_MAX_INPUT_ENTITIES]:
                target = _normalize(member.get("id") if isinstance(member, dict) else member)
                if target:
                    doc["references"].add((target, "group_member"))
        doc["signature"] = ProjectIndex._signature(doc)
        return doc

    @classmethod
    def _extract(cls, snapshot: dict) -> tuple[str, list[dict]]:
        project = project_model(snapshot)
        if not project:
            return "", []
        project_id = _safe(project.get("id") or project.get("name"), 180)
        board = project.get("board", {})
        if not isinstance(board, dict):
            boards = project.get("boards", [])
            board = boards[0] if isinstance(boards, list) and boards and isinstance(boards[0], dict) else {}
        docs = []

        # CCad currently stores design constraints as one board-level typed
        # value object rather than individually identified rule objects. Keep
        # the exact scalar settings searchable and disclose that identity model
        # instead of inventing per-rule IDs.
        raw_rules = board.get("design_rules")
        if isinstance(raw_rules, dict):
            rule_values = {}
            rule_terms = []
            for key, value in sorted(raw_rules.items()):
                if not isinstance(key, str) or not re.fullmatch(r"[a-z][a-z0-9_]{0,63}", key):
                    continue
                if isinstance(value, bool):
                    rule_value = value
                elif isinstance(value, (int, float)) and math.isfinite(value) and abs(value) <= 1e12:
                    rule_value = value
                else:
                    continue
                rule_values[key] = rule_value
                phrase = key.removesuffix("_nm").removesuffix("_degrees").replace("_", " ")
                phrase = re.sub(r"\bmin\b", "minimum", phrase)
                phrase = re.sub(r"\bmax\b", "maximum", phrase)
                rule_terms.extend((key.replace("_", " "), phrase))
            if rule_values:
                rule_doc = cls._make_doc(
                    "design_rules", {"id": "board-design-rules", "name": "Design rules"},
                    aliases=("id", "name"), extra_text=rule_terms)
                if rule_doc is not None:
                    rule_doc["fields"]["design_rules"] = rule_values
                    rule_doc["aliases"].update(_normalize(term) for term in rule_terms
                                                if _normalize(term))
                    rule_doc["tokens"] = Counter(
                        token.casefold() for token in _WORD.findall(
                            rule_doc["text"] + " " + " ".join(rule_terms))
                        if len(token) <= 80)
                    rule_doc["signature"] = cls._signature(rule_doc)
                    docs.append(rule_doc)

        def add(kind, items, **kwargs):
            for item in _iter_dicts(items):
                if len(docs) >= _MAX_INPUT_ENTITIES:
                    break
                doc = cls._make_doc(kind, item, **kwargs)
                if doc is not None:
                    docs.append(doc)
                    if len(docs) >= _MAX_INPUT_ENTITIES:
                        return

        add("layer", board.get("layers"), aliases=("id", "name"), extra_text=("copper layer",))
        add("footprint", board.get("footprints"), id_key="reference",
            aliases=("id", "reference", "value", "footprint_name"))
        add("pad", board.get("pads"), aliases=("id", "pin_name", "pin_number"))
        add("track", board.get("tracks"), aliases=("id", "source_route_request_id"))
        add("track_arc", board.get("track_arcs"), aliases=("id",))
        add("via", board.get("vias"), aliases=("id", "start_layer_id", "end_layer_id"))
        add("zone", board.get("zones"), aliases=("id", "name"),
            extra_text=("copper zone",))
        for kind, key in (("graphic", "graphics"), ("board_text", "texts"),
                          ("dimension", "dimensions"), ("keepout", "keepouts"),
                          ("route_request", "route_requests"), ("board_group", "groups"),
                          ("target", "targets"), ("barcode", "barcodes"),
                          ("board_table", "tables"),
                          ("placement_region", "placement_regions"),
                          ("reference_image", "reference_images"),
                          ("teardrop", "teardrops")):
            add(kind, board.get(key), aliases=("id", "name", "text", "kind", "net_id"))

        # The typed board stores net IDs on copper objects instead of in a
        # separate board-net table. Materialize one searchable node per ID so
        # board-only projects can resolve a net query and expand to members.
        # This indexes native association only; it does not claim continuity.
        board_net_kinds = {"footprint", "pad", "track", "track_arc", "via", "zone",
                           "graphic", "board_text", "dimension", "keepout",
                           "route_request", "board_group", "target", "barcode",
                           "board_table", "placement_region", "reference_image",
                           "teardrop"}
        board_net_ids = sorted({doc["fields"].get("net_id", "") for doc in docs
                                if doc["fields"]["kind"] in board_net_kinds and
                                doc["fields"].get("net_id")})
        for net_id in board_net_ids:
            net_doc = cls._make_doc("board_net", {"id": net_id, "net_id": net_id},
                                    aliases=("id", "net_id"),
                                    extra_text=("PCB board net",))
            if net_doc is not None and len(docs) < _MAX_INPUT_ENTITIES:
                docs.append(net_doc)

        schematic_sources = [project]
        schematics = project.get("schematics", [])
        if isinstance(schematics, list):
            schematic_sources.extend(item for item in schematics[:64] if isinstance(item, dict))
        for schematic in schematic_sources:
            source_sheet_id = (_safe(schematic.get("id") or schematic.get("name"), 120)
                               if schematic is not project else "")
            page_name = _safe(schematic.get("name") or source_sheet_id, 140)
            if source_sheet_id:
                page = cls._make_doc(
                    "schematic_page", {"id": source_sheet_id, "name": page_name},
                    aliases=("id", "name"), sheet_id=source_sheet_id,
                    extra_text=("schematic sheet page",))
                if page is not None and len(docs) < _MAX_INPUT_ENTITIES:
                    docs.append(page)
            symbols = schematic.get("components", schematic.get("symbols", []))
            add("schematic_symbol", symbols,
                aliases=("id", "reference", "part", "value", "lib_id"),
                sheet_id=source_sheet_id,
                extra_text=("schematic symbol",))
            add("schematic_net", schematic.get("nets"), aliases=("id", "name"),
                sheet_id=source_sheet_id,
                extra_text=("schematic net",))
            add("schematic_wire", schematic.get("wires"), aliases=("id", "net_id"),
                sheet_id=source_sheet_id)
            add("schematic_label", schematic.get("labels"), aliases=("id", "text", "net_id"),
                sheet_id=source_sheet_id)
            for kind, key in (("power_symbol", "power_symbols"), ("schematic_bus", "buses"),
                              ("schematic_constraint", "constraints"),
                              ("schematic_group", "groups"), ("schematic_sheet", "sheets"),
                              ("schematic_text", "texts")):
                add(kind, schematic.get(key), aliases=("id", "name", "text", "value", "net_id"),
                    sheet_id=source_sheet_id,
                    parent_sheet_id=(source_sheet_id if kind == "schematic_sheet" else ""))

        diagnostics = snapshot.get("project_diagnostics", [])
        if isinstance(diagnostics, list):
            duplicate_ids: dict[str, int] = defaultdict(int)
            for item in diagnostics[:256]:
                if not isinstance(item, dict):
                    continue
                safe_item = {key: item.get(key) for key in
                             ("engine", "severity", "code", "message", "object_id")}
                digest = hashlib.sha256(json.dumps(
                    safe_item, sort_keys=True, separators=(",", ":")).encode()).hexdigest()[:16]
                duplicate_ids[digest] += 1
                engine = _safe(item.get("engine"), 16) or "diagnostic"
                diagnostic = cls._make_doc(
                    "project_diagnostic",
                    {**safe_item, "id": f"{engine}:{digest}:{duplicate_ids[digest]}"},
                    aliases=("id", "code", "object_id"),
                    extra_text=("project diagnostic",))
                if diagnostic is not None and len(docs) < _MAX_INPUT_ENTITIES:
                    docs.append(diagnostic)

        # Build the membership lookup once; scanning every source net for every
        # indexed net makes large multi-sheet projects quadratic.
        members_by_net: dict[str, list[dict]] = defaultdict(list)
        seen_members: set[tuple[str, str, str]] = set()
        member_count = 0
        for source in schematic_sources:
            for net in _iter_dicts(source.get("nets")):
                net_id = _safe(net.get("id"), 120)
                if not net_id:
                    continue
                for member in _iter_dicts(net.get("members")):
                    component_id = _safe(member.get("component_id"), 120)
                    pin_name = _safe(member.get("pin_name"), 100)
                    key = (net_id, component_id, pin_name)
                    if not component_id or not pin_name or key in seen_members:
                        continue
                    seen_members.add(key)
                    members_by_net[net_id].append(member)
                    member_count += 1
                    if member_count >= _MAX_INPUT_ENTITIES:
                        break
                if member_count >= _MAX_INPUT_ENTITIES:
                    break
            if member_count >= _MAX_INPUT_ENTITIES:
                break

        # Schematic net.members points at serialized component IDs and pins;
        # preserve each declared member as a separate searchable logical pin.
        symbols = {}
        for symbol in docs:
            if symbol["fields"]["kind"] != "schematic_symbol":
                continue
            for key in ("id", "reference"):
                value = _normalize(symbol["fields"].get(key, ""))
                if value:
                    symbols[value] = symbol
        for doc in docs:
            if doc["fields"]["kind"] != "schematic_net":
                continue
            net_id = doc["fields"]["id"]
            for member in members_by_net.get(net_id, ()):
                component_id = _safe(member.get("component_id"), 120)
                pin_name = _safe(member.get("pin_name"), 100)
                symbol = symbols.get(_normalize(component_id))
                symbol_fields = symbol["fields"] if symbol else {}
                reference = _safe(symbol_fields.get("reference"), 120) or component_id
                symbol_id = _safe(symbol_fields.get("id"), 120) or component_id
                pin_doc = cls._make_doc(
                    "schematic_pin",
                    {"id": f"{net_id}:{component_id}:{pin_name}",
                     "component_id": reference, "symbol_id": symbol_id,
                     "reference": reference, "pin_name": pin_name,
                     "net_id": net_id},
                    aliases=("id", "pin_name", "component_id", "symbol_id"),
                    component_id=reference, net_id=net_id)
                if pin_doc is not None and len(docs) < _MAX_INPUT_ENTITIES:
                    docs.append(pin_doc)
                    doc["components"].update(pin_doc["components"])
        for doc in docs:
            doc["signature"] = cls._signature(doc)
        return project_id, docs

    def _remove(self, uid: str):
        doc = self._docs.pop(uid, None)
        if doc is None:
            return
        for alias in doc["aliases"]:
            ids = self._aliases.get(alias)
            if ids:
                ids.discard(uid)
                if not ids:
                    self._aliases.pop(alias, None)
                    for token in _WORD.findall(alias):
                        aliases = self._alias_tokens.get(token.casefold())
                        if aliases:
                            aliases.discard(alias)
                            if not aliases:
                                self._alias_tokens.pop(token.casefold(), None)
        for term in self._doc_terms.pop(uid, {}):
            posting = self._postings.get(term)
            if posting:
                posting.pop(uid, None)
                if not posting:
                    self._postings.pop(term, None)
        self._doc_lengths.pop(uid, None)
        for field, values in (("net", (doc["net"],)),
                              ("layer", doc["layers"])):
            buckets = getattr(self, f"_{field}_docs")
            for value in values:
                bucket = buckets.get(value) if value else None
                if bucket:
                    bucket.discard(uid)
                    if not bucket:
                        buckets.pop(value, None)
        for value in doc["sheets"]:
            bucket = self._sheet_docs.get(value)
            if bucket:
                bucket.discard(uid)
                if not bucket:
                    self._sheet_docs.pop(value, None)
        for target, relation in doc["references"]:
            bucket = self._reference_docs.get(target)
            if bucket:
                bucket.discard((uid, relation))
                if not bucket:
                    self._reference_docs.pop(target, None)
        for value in doc["components"]:
            bucket = self._component_docs.get(value)
            if bucket:
                bucket.discard(uid)
                if not bucket:
                    self._component_docs.pop(value, None)
        self._remove_spatial(uid)

    def _remove_spatial(self, uid: str):
        for key in self._spatial_keys.pop(uid, ()):
            cell = self._spatial_cells.get(key)
            if cell:
                cell.discard(uid)
                if not cell:
                    self._spatial_cells.pop(key, None)
        self._spatial_global.discard(uid)

    def _add_spatial(self, doc: dict):
        bounds = doc["fields"].get("bounds_mm")
        if not bounds:
            return
        cell = self.CELL_MM
        x0, y0 = math.floor(bounds["min_x_mm"] / cell), math.floor(bounds["min_y_mm"] / cell)
        x1, y1 = math.floor(bounds["max_x_mm"] / cell), math.floor(bounds["max_y_mm"] / cell)
        if (x1 - x0 + 1) * (y1 - y0 + 1) > 256:
            self._spatial_global.add(doc["uid"])
            self._spatial_keys[doc["uid"]] = ()
            return
        keys = tuple((x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1))
        self._spatial_keys[doc["uid"]] = keys
        for key in keys:
            self._spatial_cells[key].add(doc["uid"])

    def _insert(self, doc: dict):
        uid = doc["uid"]
        self._docs[uid] = doc
        for alias in doc["aliases"]:
            self._aliases[alias].add(uid)
            for token in _WORD.findall(alias):
                self._alias_tokens[token.casefold()].add(alias)
        self._doc_terms[uid] = doc["tokens"]
        self._doc_lengths[uid] = sum(doc["tokens"].values())
        for term, count in doc["tokens"].items():
            self._postings[term][uid] = count
        if doc["net"]:
            self._net_docs[doc["net"]].add(uid)
        for value in doc["layers"]:
            self._layer_docs[value].add(uid)
        for value in doc["components"]:
            self._component_docs[value].add(uid)
        for value in doc["sheets"]:
            self._sheet_docs[value].add(uid)
        for target, relation in doc["references"]:
            self._reference_docs[target].add((uid, relation))
        self._add_spatial(doc)

    def _sync(self, snapshot: dict) -> dict:
        project_id, incoming_docs = self._extract(snapshot)
        if not project_id:
            # Native snapshots in older projects can omit project.id. A stable
            # private content-derived key still gives safe, project-isolated state.
            project_id = "opaque:" + hashlib.sha256(json.dumps(
                project_model(snapshot), sort_keys=True, separators=(",", ":")
            ).encode()).hexdigest()[:24]
        incoming = {doc["uid"]: doc for doc in incoming_docs}
        content_revision = hashlib.sha256(json.dumps(
            sorted((uid, doc["signature"]) for uid, doc in incoming.items()),
            separators=(",", ":")).encode()).hexdigest()[:24]
        if self._has_snapshot and project_id == self._project_id and content_revision == self._revision:
            return {"index_state": "cached", "revision": content_revision,
                    "inserted_count": 0, "updated_count": 0, "removed_count": 0,
                    "unchanged_count": len(self._docs), "total_entities": len(self._docs)}

        reset = not self._has_snapshot or project_id != self._project_id
        old_uids = set() if reset else set(self._docs)
        incoming_uids = set(incoming)
        removed = old_uids - incoming_uids
        if reset:
            removed = set(self._docs)
        for uid in removed:
            self._remove(uid)
        inserted = updated = unchanged = 0
        for uid, doc in incoming.items():
            old = self._docs.get(uid)
            if old is not None and old["signature"] == doc["signature"]:
                unchanged += 1
                continue
            if old is not None:
                self._remove(uid)
                updated += 1
            else:
                inserted += 1
            self._insert(doc)
        self._project_id = project_id
        self._revision = content_revision
        self._has_snapshot = True
        return {"index_state": "built" if reset else "incremental",
                "revision": content_revision, "inserted_count": inserted,
                "updated_count": updated, "removed_count": len(removed),
                "unchanged_count": unchanged, "total_entities": len(self._docs)}

    @staticmethod
    def _query_tokens(query: str) -> list[str]:
        return list(dict.fromkeys(token.casefold() for token in _WORD.findall(query)
                                 if len(token) > 1 and token.casefold() not in _STOP))[:48]

    def _exact(self, query: str, extras=()) -> set[str]:
        normalized = _normalize(query)
        found = set()
        tokens = {token.casefold() for token in _WORD.findall(normalized)}
        candidates = {alias for token in tokens
                      for alias in self._alias_tokens.get(token, ())}
        matched = []
        for alias in candidates:
            uids = self._aliases.get(alias, ())
            if len(alias) < 2:
                continue
            if re.search(r"(?<![\w])" + re.escape(alias) + r"(?![\w])", normalized):
                matched.append((alias, uids))
        # A full colon-delimited pin identity outranks its embedded net-name
        # alias, including when natural-language words precede the identity.
        structured = [(alias, uids) for alias, uids in matched if ":" in alias]
        for _, uids in structured or matched:
            found.update(uids)
        for value in extras:
            found.update(self._aliases.get(_normalize(value), ()))
        return found

    def _bm25(self, query: str, limit: int) -> list[tuple[str, float]]:
        terms = self._query_tokens(query)
        count = len(self._docs)
        if not terms or count == 0:
            return []
        average = sum(self._doc_lengths.values()) / max(1, count)
        k1, b = 1.5, 0.75
        scores: dict[str, float] = defaultdict(float)
        for term in terms:
            posting = self._postings.get(term, {})
            df = len(posting)
            if not df:
                continue
            idf = math.log(1.0 + (count - df + 0.5) / (df + 0.5))
            for uid, frequency in posting.items():
                length = self._doc_lengths[uid]
                scores[uid] += idf * (frequency * (k1 + 1.0)) / (
                    frequency + k1 * (1.0 - b + b * length / max(1.0, average)))
        # Resolve equal lexical scores in favor of physical board objects when
        # a prompt mentions a board-library term as well as a symbol value.
        return sorted(((uid, score + (0.2 if self._docs[uid]["fields"]["kind"] in
                                      {"footprint", "pad", "track", "track_arc", "via", "zone"}
                                      else 0.0)) for uid, score in scores.items()), key=lambda item: (
            -item[1],
            0 if self._docs[item[0]]["fields"]["kind"] in
            {"footprint", "pad", "track", "track_arc", "via", "zone"} else 1,
            item[0]))[:limit]

    def revision(self, project_id: str = "") -> str:
        """Return the active typed-project content revision, not a UI epoch."""
        if project_id and self._project_id not in (project_id, "opaque:" + project_id):
            return ""
        return self._revision

    @staticmethod
    def _distance(point, bounds) -> float:
        dx = max(bounds["min_x_mm"] - point[0], 0.0, point[0] - bounds["max_x_mm"])
        dy = max(bounds["min_y_mm"] - point[1], 0.0, point[1] - bounds["max_y_mm"])
        return math.hypot(dx, dy)

    def _spatial(self, point, radius: float) -> list[tuple[str, float]]:
        cell = self.CELL_MM
        x0, y0 = math.floor((point[0] - radius) / cell), math.floor((point[1] - radius) / cell)
        x1, y1 = math.floor((point[0] + radius) / cell), math.floor((point[1] + radius) / cell)
        candidates = set(self._spatial_global)
        if (x1 - x0 + 1) * (y1 - y0 + 1) > 1024:
            candidates.update(self._docs)
        else:
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    candidates.update(self._spatial_cells.get((x, y), ()))
        scored = []
        for uid in candidates:
            bounds = self._docs[uid]["fields"].get("bounds_mm")
            if bounds:
                distance = self._distance(point, bounds)
                if distance <= radius:
                    scored.append((uid, distance))
        return sorted(scored, key=lambda item: (item[1], item[0]))

    def _relationship_neighbors(self, seeds: set[str]) -> dict[str, set[str]]:
        found: dict[str, set[str]] = defaultdict(set)
        for uid in seeds:
            doc = self._docs.get(uid)
            if not doc:
                continue
            for neighbor in self._net_docs.get(doc["net"], ()) if doc["net"] else ():
                if neighbor != uid and neighbor not in seeds:
                    relation = "same_net"
                    other_kind = self._docs.get(neighbor, {}).get(
                        "fields", {}).get("kind", "")
                    schematic_kinds = {"schematic_net", "schematic_pin",
                                       "schematic_symbol", "schematic_wire",
                                       "schematic_label"}
                    if doc["fields"]["kind"] in schematic_kinds and \
                            other_kind in schematic_kinds:
                        relation = "logical_net_member"
                    elif "board_net" in {doc["fields"]["kind"], other_kind}:
                        relation = ("matching_net_id" if
                                    doc["fields"]["kind"] in schematic_kinds or
                                    other_kind in schematic_kinds else "board_net_member")
                    elif doc["fields"]["kind"] in schematic_kinds or \
                            other_kind in schematic_kinds:
                        relation = "matching_net_id"
                    found[neighbor].add(relation)
            for layer in doc["layers"]:
                for neighbor in self._layer_docs.get(layer, ()):
                    if neighbor != uid and neighbor not in seeds:
                        found[neighbor].add("same_layer")
            for value in doc["components"]:
                for neighbor in self._component_docs.get(value, ()):
                    if neighbor != uid and neighbor not in seeds:
                        other_kind = self._docs.get(neighbor, {}).get(
                            "fields", {}).get("kind", "")
                        relationship = ("logical_net_member"
                                        if doc["fields"]["kind"] == "schematic_net" and
                                        other_kind in {"schematic_pin", "schematic_symbol"}
                                        else "same_component")
                        found[neighbor].add(relationship)
            for sheet in doc["sheets"]:
                for neighbor in self._sheet_docs.get(sheet, ()):
                    if neighbor != uid and neighbor not in seeds:
                        found[neighbor].add("same_schematic_sheet")
            for target, relation in doc["references"]:
                for alias_uid in self._aliases.get(target, ()):
                    if alias_uid != uid and alias_uid not in seeds:
                        found[alias_uid].add(relation)
            identity_keys = set(doc["aliases"])
            identity = _normalize(doc["fields"].get("id"))
            if identity:
                identity_keys.add(identity)
            for identity_key in identity_keys:
                for neighbor, relation in self._reference_docs.get(identity_key, ()):
                    if neighbor != uid and neighbor not in seeds:
                        found[neighbor].add(relation)
        return found

    def retrieve(self, snapshot: Any, query: str, *, active_layer: str = "",
                 active_net: str = "", selected_objects=(), limit: int = 10) -> dict:
        if isinstance(snapshot, str):
            try:
                snapshot = json.loads(snapshot)
            except (TypeError, json.JSONDecodeError):
                snapshot = {}
        if not isinstance(snapshot, dict) or not project_model(snapshot):
            return {"available": False, "reason": "typed_project_snapshot_unavailable",
                    "entities": [], "characters": 0, "revision": "",
                    "relationship_semantics": "shared_net_association_only",
                    "board_net_semantics":
                        "native_net_id_association_not_physical_continuity",
                    "logical_net_semantics":
                        "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity",
                    "spatial_semantics": "axis_aligned_bounds_distance_only",
                    "stats": {"index_state": "unavailable", "total_entities": 0,
                              "exact_match_count": 0, "lexical_match_count": 0,
                              "relationship_match_count": 0, "spatial_match_count": 0,
                              "omitted_count": 0}}
        stats = self._sync(snapshot)
        query = _safe(query, 3072)
        limit = max(1, min(self.max_entities, int(limit)))
        extras = [active_layer, active_net]
        extras.extend(list(selected_objects or ())[:32])
        exact_ids = self._exact(query, extras)
        lexical = self._bm25(query, limit * 2)
        lexical_ids = {uid for uid, _score in lexical}
        seeds = exact_ids
        related = self._relationship_neighbors(seeds)

        points = _coordinates(query)
        nearby = bool(re.search(r"\b(near|around|nearby|within|radius|close\s+to|beside)\b",
                                query, re.IGNORECASE))
        if nearby and not points:
            for uid in sorted(exact_ids)[:4]:
                position = self._docs[uid]["fields"].get("position_mm")
                if position:
                    points.append((position["x"], position["y"]))
        radius = _radius(query)
        spatial_scores: dict[str, float] = {}
        anchor_ids = exact_ids if nearby and not _coordinates(query) else set()
        for point in points[:4]:
            for uid, distance in self._spatial(point, radius)[:limit * 2]:
                if uid not in anchor_ids:
                    spatial_scores[uid] = min(spatial_scores.get(uid, math.inf), distance)

        scores: dict[str, tuple[float, str, str, float]] = {}
        for uid in exact_ids:
            scores[uid] = (1000.0, "exact", "", 0.0)
        for rank, (uid, score) in enumerate(lexical):
            candidate = (500.0 + score - rank * 0.001, "lexical", "", 0.0)
            if uid not in scores or candidate[0] > scores[uid][0]:
                scores[uid] = candidate
        for uid, relations in related.items():
            relation = sorted(relations)[0]
            if uid not in scores:
                scores[uid] = (100.0, "relationship", relation, 0.0)
            elif scores[uid][1] == "lexical":
                prior = scores[uid]
                scores[uid] = (prior[0], prior[1], relation, prior[3])
        for uid, distance in spatial_scores.items():
            if uid not in scores:
                scores[uid] = (50.0 - distance, "spatial", "", distance)

        chosen = sorted(scores.items(), key=lambda item: (-item[1][0], item[0]))
        output, used_chars = [], 0
        for rank, (uid, (_score, mode, relation, distance)) in enumerate(chosen, 1):
            fields = self._docs[uid]["fields"]
            item = {key: value for key, value in fields.items()
                    if key in {"id", "kind", "reference", "value", "name", "part",
                               "pin_name", "pin_number", "type", "net_id", "membership_kind", "layer_id",
                               "layer_ids", "start_layer_id", "end_layer_id",
                               "component_id", "symbol_id", "position_mm", "bounds_mm",
                               "sheet_id", "parent_sheet_id", "code", "severity",
                               "message", "engine", "object_id"}}
            if "design_rules" in fields:
                item["design_rules"] = fields["design_rules"]
            for key in ("anchor_pad_id", "anchor_via_id", "anchor_track_id",
                        "from_object_id", "to_object_id", "preferred_layer_id"):
                if key in fields:
                    item[key] = fields[key]
            item["retrieval"] = mode
            item["rank"] = rank
            if relation:
                item["relationship"] = relation
                item["relationships"] = sorted(related.get(uid, ()))
            if mode == "spatial":
                item["distance_mm"] = round(distance, 4)
            encoded_size = len(json.dumps(item, ensure_ascii=False, separators=(",", ":")))
            if len(output) >= limit or used_chars + encoded_size > self.max_chars:
                continue
            used_chars += encoded_size
            output.append(item)

        stats.update({"exact_match_count": len(exact_ids),
                      "lexical_match_count": len(lexical_ids),
                      "relationship_match_count": len(related),
                      "spatial_match_count": len(spatial_scores),
                      "omitted_count": max(0, len(scores) - len(output)),
                      "total_entities": len(self._docs)})
        return {"available": True, "reason": "", "revision": self._revision,
                "entities": output, "characters": used_chars, "stats": stats,
                "relationship_semantics": "shared_net_association_only",
                "board_net_semantics":
                    "native_net_id_association_not_physical_continuity",
                "logical_net_semantics":
                    "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity",
                "spatial_semantics": "axis_aligned_bounds_distance_only",
                "explicit_reference_semantics":
                    "serialized object references and declared schematic page membership only",
                "search_method": "exact_alias_bm25_relationship_spatial"}
