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
_SENSITIVE_PROPERTY = re.compile(
    r"(?:api[_-]?key|secret|password|token|authorization|credential)", re.IGNORECASE)
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
_BOARD_GEOMETRY_KINDS = frozenset({
    "footprint", "pad", "track", "track_arc", "via", "zone", "graphic",
    "board_text", "dimension", "keepout", "route_request", "board_group",
    "target", "barcode", "board_table", "placement_region", "reference_image",
    "teardrop", "board_net"})
_SCHEMATIC_GEOMETRY_KINDS = frozenset({
    "schematic_symbol", "schematic_pin", "schematic_wire", "schematic_label",
    "schematic_net", "schematic_page", "schematic_sheet", "schematic_text",
    "schematic_textbox",
    "schematic_graphic", "schematic_junction", "schematic_no_connect",
    "schematic_marker", "schematic_bus_entry", "schematic_bus", "schematic_rule_area",
    "schematic_table", "schematic_bitmap", "schematic_group", "power_symbol"})


def _normalize(value: Any) -> str:
    return " ".join(str(value or "").casefold().split())


def _safe(value: Any, limit: int = 120) -> str:
    if value is None:
        return ""
    text = str(value).strip()
    if not text or _SECRET.search(text):
        return ""
    return text[:limit]


def _relative_sheet_path(value: Any) -> str:
    """Keep typed sheet identity searchable without exporting host file paths."""
    path = _safe(value, 240).replace("\\", "/")
    if (not path or path.startswith("/") or path.startswith("//") or
            re.match(r"^[A-Za-z]:", path)):
        return ""
    return path


def _typed_properties(item: dict) -> list[dict]:
    """Return bounded scalar properties from native symbol fields or metadata."""
    values: list[dict] = []
    raw = item.get("properties")
    if isinstance(raw, dict):
        entries = ((key, value, None) for key, value in
                   sorted(raw.items(), key=lambda pair: str(pair[0]))[:32])
    elif isinstance(raw, list):
        entries = ((prop.get("name", prop.get("key")),
                    prop.get("value", prop.get("text")),
                    prop.get("visible") if isinstance(prop.get("visible"), bool) else None)
                   for prop in raw[:32] if isinstance(prop, dict))
    else:
        entries = iter(())

    def append(name: Any, value: Any, visible: bool | None):
        clean_name = _safe(name, 80)
        clean_value = _safe(value, 160) if isinstance(value, (str, int, float, bool)) else ""
        if not clean_name or _SENSITIVE_PROPERTY.search(clean_name) or not clean_value:
            return
        prop: dict[str, Any] = {"name": clean_name, "value": clean_value}
        if visible is not None:
            prop["visible"] = visible
        values.append(prop)

    for name, value, visible in entries:
        append(name, value, visible)

    # CCad's authoritative schematic serializer calls instantiated symbol
    # properties "fields" and stores their user-facing text under "text".
    for field in item.get("fields", [])[:32] if isinstance(item.get("fields"), list) else ():
        if isinstance(field, dict):
            append(field.get("name"), field.get("text"),
                   field.get("visible") if isinstance(field.get("visible"), bool) else None)
    return values[:32]


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


def _query_bounds(query: str) -> dict | None:
    """Read an explicit two-corner rectangle query; never infer a box from prose."""
    if not re.search(r"\b(?:bbox|bounding\s+box|box|rectangle|rectangular\s+region)\b",
                     query, re.IGNORECASE):
        return None
    points = _coordinates(query)
    if len(points) < 2:
        return None
    first, second = points[:2]
    return {"min_x_mm": min(first[0], second[0]),
            "min_y_mm": min(first[1], second[1]),
            "max_x_mm": max(first[0], second[0]),
            "max_y_mm": max(first[1], second[1])}


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
        self._source_signature = ""
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
                           ("description", item.get("description")),
                           ("library_description", item.get("library_description")),
                           ("footprint_name", item.get("footprint_name")),
                           ("lib_id", item.get("lib_id")),
                           ("sheet_path", item.get("sheet_path")),
                           ("title", item.get("title")),
                           ("text", item.get("text")),
                           ("notes", item.get("notes")),
                           ("target", item.get("target")),
                           ("pin_name", item.get("pin_name")),
                           ("pin_number", item.get("pin_number")),
                           ("electrical_type", item.get("electrical_type")),
                           ("graphical_style", item.get("graphical_style")),
                           ("orientation", item.get("orientation")),
                           ("identity_source", item.get("identity_source")),
                           ("type", item.get("kind", item.get("type"))),
                           ("code", item.get("code")),
                           ("severity", item.get("severity")),
                           ("message", item.get("message")),
                           ("engine", item.get("engine")),
                           ("object_id", item.get("object_id"))):
            clean = _safe(value, 140)
            if clean:
                fields[key] = clean
        for key in ("locked", "visible"):
            if isinstance(item.get(key), bool):
                fields[key] = item[key]
        if (isinstance(item.get("unit"), int) and
                not isinstance(item.get("unit"), bool) and 0 <= item["unit"] <= 1024):
            fields["symbol_unit"] = item["unit"]
        raw_sheet_path = item.get("sheet_path", item.get("file_path"))
        sheet_path = _relative_sheet_path(raw_sheet_path)
        if kind == "schematic_sheet" and sheet_path:
            fields["sheet_path"] = sheet_path
        properties = _typed_properties(item)
        if properties:
            fields["properties"] = properties
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
                fields["membership_kind"] = (
                    _safe(item.get("membership_kind"), 40) or "schematic_net_member")
        elif kind == "schematic_pin":
            fields["membership_kind"] = (
                _safe(item.get("membership_kind"), 40) or "declared_pin")
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
                                 if key not in {"start_layer_id", "end_layer_id"} and
                                 not (kind == "schematic_sheet" and key == "file_path"))
        searchable_values.extend(fields.get(key, "") for key in
                                 ("reference", "value", "name", "part", "description",
                                  "library_description", "footprint_name", "lib_id",
                                  "sheet_path", "title", "text", "notes", "target",
                                  "pin_name", "pin_number", "electrical_type",
                                  "graphical_style", "orientation",
                                  "code", "severity", "message", "engine", "object_id"))
        for prop in properties:
            searchable_values.extend((prop["name"], prop["value"]))
        exact_values = [object_id]
        exact_values.extend(_safe(item.get(key), 140) for key in aliases
                            if key in {"id", "reference", "name", "pin_name", "pin_number"}
                            or (kind == "schematic_sheet" and key == "sheet_path")
                            or (kind == "project_diagnostic" and
                                key in {"code", "object_id"}))
        if kind == "schematic_sheet" and sheet_path:
            exact_values.append(sheet_path)
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
        declared_pin_sources: list[tuple[str, dict]] = []
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
            declared_pin_sources.extend(
                (source_sheet_id, {**symbol, "symbol_id": _safe(symbol.get("id"), 120),
                                   "reference": _safe(symbol.get("reference"), 120)})
                for symbol in _iter_dicts(symbols)
                if isinstance(symbol.get("pins"), list))
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
                              ("schematic_text", "texts"),
                              ("schematic_textbox", "textboxes"),
                              ("schematic_graphic", "graphics"),
                              ("schematic_junction", "junctions"),
                              ("schematic_no_connect", "no_connects"),
                              ("schematic_marker", "markers"),
                              ("schematic_bus_entry", "bus_entries"),
                              ("schematic_rule_area", "rule_areas"),
                              ("schematic_table", "tables"),
                              ("schematic_bitmap", "bitmaps")):
                add(kind, schematic.get(key), aliases=("id", "name", "text", "value", "net_id"),
                    sheet_id=source_sheet_id,
                    parent_sheet_id=(source_sheet_id if kind == "schematic_sheet" else ""))
            # Table cell text is useful for retrieval; embedded bitmap data is
            # deliberately excluded from both the index and provider context.
            for table in _iter_dicts(schematic.get("tables")):
                cell_text = [_safe(cell.get("text"), 140)
                             for cell in _iter_dicts(table.get("cells"))]
                cell_text = [value for value in cell_text if value]
                if cell_text:
                    table_id = _safe(table.get("id"), 120)
                    table_doc = next((doc for doc in reversed(docs)
                                      if doc["fields"]["kind"] == "schematic_table" and
                                      doc["fields"]["id"] == table_id), None)
                    if table_doc is not None:
                        table_doc["fields"]["text"] = " | ".join(cell_text)[:400]
                        table_doc["text"] = (table_doc["text"] + " " +
                                              table_doc["fields"]["text"])[:1200]
                        table_doc["tokens"].update(Counter(
                            token.casefold() for token in _WORD.findall(
                                table_doc["fields"]["text"]) if len(token) <= 80))

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

        # Build typed declared-pin records before linking net membership so
        # unconnected pins remain visible and connected pins retain electrical
        # metadata from their source symbol.
        declared_pin_docs: list[dict] = []
        declared_by_component_pin: dict[tuple[str, str], list[dict]] = defaultdict(list)
        for sheet_id, symbol in declared_pin_sources:
            symbol_id = _safe(symbol.get("symbol_id") or symbol.get("id"), 120)
            reference = _safe(symbol.get("reference"), 120)
            unit = symbol.get("unit", 1)
            for pin in _iter_dicts(symbol.get("pins")):
                pin_name = _safe(pin.get("name"), 100)
                pin_number = _safe(pin.get("number"), 100)
                if not pin_name and not pin_number:
                    continue
                source_pin_id = _safe(pin.get("id"), 120)
                identity_part = source_pin_id or pin_number or pin_name
                identity = f"declared:{symbol_id or reference}:{unit}:{identity_part}"
                pin_item = {
                    **pin, "id": identity, "symbol_id": symbol_id,
                    "component_id": reference, "reference": reference,
                    "pin_name": pin_name, "pin_number": pin_number,
                    "unit": unit, "membership_kind": "declared_pin",
                    "identity_source": ("native_pin_id" if source_pin_id else
                        "derived_from_symbol_unit_and_pin_number_or_name"),
                }
                pin_doc = cls._make_doc(
                    "schematic_pin", pin_item,
                    aliases=("id", "pin_name", "pin_number", "symbol_id"),
                    component_id=reference, sheet_id=sheet_id)
                if pin_doc is None:
                    continue
                declared_pin_docs.append(pin_doc)
                pin_key = _normalize(pin_name)
                if pin_key:
                    for component_key in {symbol_id, reference}:
                        normalized_component = _normalize(component_key)
                        if normalized_component:
                            declared_by_component_pin[
                                (normalized_component, pin_key)].append(pin_doc)

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

        # Schematic net.members points at serialized component IDs and pins.
        # Attach a membership to a declared pin only when that identity maps
        # unambiguously; retain unresolved native members as separate records.
        symbols = {}
        for symbol in docs:
            if symbol["fields"]["kind"] != "schematic_symbol":
                continue
            for key in ("id", "reference"):
                value = _normalize(symbol["fields"].get(key, ""))
                if value:
                    symbols[value] = symbol
        member_claims: dict[str, list[tuple[str, dict]]] = defaultdict(list)
        unresolved_members: list[tuple[str, dict, bool]] = []
        for net_id, members in members_by_net.items():
            for member in members:
                component_id = _safe(member.get("component_id"), 120)
                pin_name = _safe(member.get("pin_name"), 100)
                symbol = symbols.get(_normalize(component_id))
                component_keys = {component_id}
                if symbol:
                    component_keys.update((symbol["fields"].get("id", ""),
                                           symbol["fields"].get("reference", "")))
                candidates = {pin_doc["uid"]: pin_doc
                              for component_key in component_keys
                              for pin_doc in declared_by_component_pin.get(
                                  (_normalize(component_key), _normalize(pin_name)), ())}
                if len(candidates) == 1:
                    pin_doc = next(iter(candidates.values()))
                    member_claims[pin_doc["uid"]].append((net_id, member))
                else:
                    unresolved_members.append((net_id, member, len(candidates) > 1))

        for pin_doc in declared_pin_docs:
            claims = member_claims.get(pin_doc["uid"], ())
            claimed_nets = sorted({net_id for net_id, _member in claims})
            if len(claimed_nets) == 1:
                net_id = claimed_nets[0]
                symbol_id = pin_doc["fields"].get("symbol_id", "")
                pin_name = pin_doc["fields"].get("pin_name", "")
                component_id = pin_doc["fields"].get("reference", "") or \
                    pin_doc["fields"].get("component_id", "")
                # Preserve the historical net-member ID while enriching its
                # payload with the exact declaration fields.
                pin_doc["fields"]["id"] = f"{net_id}:{component_id}:{pin_name}"
                pin_doc["uid"] = f"schematic_pin:{pin_doc['fields']['id']}"
                pin_doc["fields"]["net_id"] = net_id
                pin_doc["fields"]["membership_kind"] = "schematic_net_member"
                pin_doc["net"] = _normalize(net_id)
                pin_doc["aliases"].add(_normalize(net_id))
                pin_doc["tokens"].update(Counter(
                    token.casefold() for token in _WORD.findall(net_id)))
                pin_doc["text"] = (pin_doc["text"] + " " + net_id)[:1200]
            elif claimed_nets:
                pin_doc["fields"]["membership_kind"] = "ambiguous_net_membership"
                pin_doc["fields"]["candidate_net_ids"] = claimed_nets[:8]
                unresolved_members.extend((net_id, member, True)
                                          for net_id, member in claims)

        docs.extend(declared_pin_docs)
        for doc in docs:
            if doc["fields"]["kind"] != "schematic_net":
                continue
            net_id = doc["fields"]["id"]
            for member_net, member, ambiguous in unresolved_members:
                if member_net != net_id:
                    continue
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
                      "net_id": net_id,
                      "membership_kind": ("ambiguous_pin_declaration" if ambiguous
                                          else "schematic_net_member")},
                     aliases=("id", "pin_name", "component_id", "symbol_id"),
                     component_id=reference, net_id=net_id)
                if pin_doc is not None and len(docs) < _MAX_INPUT_ENTITIES:
                    docs.append(pin_doc)
                    doc["components"].update(pin_doc["components"])

        # Build functional-block search documents only from explicit native
        # group membership and serialized sheet hierarchy. These are derived
        # index records, never inferred design truth; their provenance and
        # current source revision remain visible to callers.
        initial_docs = tuple(docs)
        aliases: dict[str, list[dict]] = defaultdict(list)
        by_sheet: dict[str, list[dict]] = defaultdict(list)
        for source_doc in initial_docs:
            for alias in source_doc["aliases"]:
                aliases[alias].append(source_doc)
            for sheet in source_doc["sheets"]:
                by_sheet[sheet].append(source_doc)

        block_sources = [doc for doc in initial_docs
                         if doc["fields"]["kind"] in {"board_group", "schematic_group"}]

        def block_bounds(members):
            points = []
            for member in members:
                fields = member["fields"]
                position = fields.get("position_mm")
                if isinstance(position, dict):
                    point = _point(position)
                    if point is not None:
                        points.append(point)
                bounds = fields.get("bounds_mm")
                if isinstance(bounds, dict):
                    for x_key, y_key in (("min_x_mm", "min_y_mm"),
                                         ("max_x_mm", "max_y_mm")):
                        point = _point({"x": bounds.get(x_key), "y": bounds.get(y_key)})
                        if point is not None:
                            points.append(point)
            return _bbox(points)

        for source_doc in block_sources:
            fields = source_doc["fields"]
            source_member_ids = sorted({target for target, relation in source_doc["references"]
                                        if relation == "group_member"})[:64]
            allowed_kinds = ({"footprint", "pad", "track", "track_arc", "via", "zone",
                               "graphic", "board_text", "dimension", "keepout",
                               "route_request", "board_group", "target", "barcode",
                               "board_table", "placement_region", "reference_image",
                               "teardrop"}
                              if fields["kind"] == "board_group" else
                              {"schematic_symbol", "schematic_net", "schematic_pin",
                               "schematic_wire", "schematic_label", "schematic_page",
                               "schematic_sheet", "schematic_text", "schematic_textbox",
                               "schematic_graphic", "schematic_junction",
                               "schematic_no_connect", "schematic_marker",
                               "schematic_bus_entry", "schematic_rule_area",
                               "schematic_table"})
            member_docs = {}
            for target, relation in source_doc["references"]:
                if relation != "group_member":
                    continue
                for member in aliases.get(target, ()):
                    if (member["uid"] != source_doc["uid"] and
                            member["fields"]["kind"] in allowed_kinds):
                        member_docs[member["uid"]] = member
            block = cls._make_doc(
                "functional_block",
                {"id": f"group:{fields['kind']}:{fields['id']}",
                 "name": fields.get("name") or fields.get("text") or fields["id"]},
                aliases=("id", "name"),
                extra_text=("explicit functional block", fields.get("name", ""),
                            *(member["text"] for member in member_docs.values())))
            if block is None:
                continue
            block["fields"].update({
                "provenance": "explicit_user_group",
                "source_group_id": fields["id"],
                "member_count": len(member_docs),
                "source_member_ids": source_member_ids,
                "members": sorted({member["fields"].get("reference") or
                                   member["fields"].get("id", "")
                                   for member in member_docs.values()} - {""})[:64],
                "related_net_ids": sorted({member["fields"].get("net_id", "")
                                            for member in member_docs.values()
                                            if member["fields"].get("net_id")})[:64],
            })
            bounds = block_bounds(member_docs.values())
            if bounds:
                block["fields"]["bounds_mm"] = {
                    key: round(value, 6) for key, value in bounds.items()}
            block["references"].update((member["fields"].get("id", ""), "group_member")
                                        for member in member_docs.values())
            block["references"].discard(("", "group_member"))
            block["references"].add((fields["id"], "source_group"))
            block["text"] = (block["text"] + " " + " ".join(
                net for net in block["fields"]["related_net_ids"]))[:1200]
            block["tokens"] = Counter(token.casefold() for token in _WORD.findall(
                block["text"]) if len(token) <= 80)
            block["signature"] = cls._signature(block)
            if len(docs) < _MAX_INPUT_ENTITIES:
                docs.append(block)

        for sheet_doc in (doc for doc in initial_docs
                          if doc["fields"]["kind"] == "schematic_sheet"):
            sheet_id = sheet_doc["fields"]["id"]
            member_docs = {member["uid"]: member for member in by_sheet.get(
                _normalize(sheet_id), ()) if member["uid"] != sheet_doc["uid"]}
            if not member_docs:
                continue
            fields = sheet_doc["fields"]
            block = cls._make_doc(
                "functional_block",
                {"id": f"sheet:{sheet_id}",
                 "name": fields.get("name") or fields.get("title") or sheet_id},
                aliases=("id", "name"),
                extra_text=("schematic sheet functional block", fields.get("name", ""),
                            *(member["text"] for member in member_docs.values())))
            if block is None:
                continue
            block["fields"].update({
                "provenance": "serialized_schematic_sheet",
                "source_sheet_id": sheet_id,
                "member_count": len(member_docs),
                "source_member_ids": [],
                "members": sorted({member["fields"].get("reference") or
                                   member["fields"].get("id", "")
                                   for member in member_docs.values()} - {""})[:64],
                "related_net_ids": sorted({member["fields"].get("net_id", "")
                                            for member in member_docs.values()
                                            if member["fields"].get("net_id")})[:64],
            })
            bounds = block_bounds(member_docs.values())
            if bounds:
                block["fields"]["bounds_mm"] = {
                    key: round(value, 6) for key, value in bounds.items()}
            block["references"].update((member["fields"].get("id", ""), "group_member")
                                        for member in member_docs.values())
            block["references"].add((sheet_id, "source_sheet"))
            block["text"] = (block["text"] + " " + " ".join(
                net for net in block["fields"]["related_net_ids"]))[:1200]
            block["tokens"] = Counter(token.casefold() for token in _WORD.findall(
                block["text"]) if len(token) <= 80)
            block["signature"] = cls._signature(block)
            if len(docs) < _MAX_INPUT_ENTITIES:
                docs.append(block)

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
        project = project_model(snapshot)
        source_signature = hashlib.sha256(json.dumps(
            project, sort_keys=True, separators=(",", ":")
        ).encode()).hexdigest()
        if project:
            project_id = _safe(project.get("id") or project.get("name"), 180)
            if not project_id:
                project_id = "opaque:" + source_signature[:24]
            if (self._has_snapshot and project_id == self._project_id and
                    source_signature == self._source_signature):
                return {"index_state": "cached", "revision": self._revision,
                        "inserted_count": 0, "updated_count": 0, "removed_count": 0,
                        "unchanged_count": len(self._docs),
                        "total_entities": len(self._docs)}
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
        self._source_signature = source_signature
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

    def _spatial_box(self, bounds: dict) -> list[tuple[str, float]]:
        """Return typed entities whose indexed AABBs intersect an explicit query box."""
        cell = self.CELL_MM
        x0 = math.floor(bounds["min_x_mm"] / cell)
        y0 = math.floor(bounds["min_y_mm"] / cell)
        x1 = math.floor(bounds["max_x_mm"] / cell)
        y1 = math.floor(bounds["max_y_mm"] / cell)
        candidates = set(self._spatial_global)
        if (x1 - x0 + 1) * (y1 - y0 + 1) > 1024:
            candidates.update(self._docs)
        else:
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    candidates.update(self._spatial_cells.get((x, y), ()))
        found = []
        for uid in candidates:
            item_bounds = self._docs[uid]["fields"].get("bounds_mm")
            if not item_bounds:
                continue
            intersects = (item_bounds["min_x_mm"] <= bounds["max_x_mm"] and
                          item_bounds["max_x_mm"] >= bounds["min_x_mm"] and
                          item_bounds["min_y_mm"] <= bounds["max_y_mm"] and
                          item_bounds["max_y_mm"] >= bounds["min_y_mm"])
            if intersects:
                found.append((uid, 0.0))
        return sorted(found, key=lambda item: item[0])

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
                        if (doc["fields"]["kind"], other_kind) in {
                                ("schematic_pin", "schematic_symbol"),
                                ("schematic_symbol", "schematic_pin")}:
                            pin = doc if doc["fields"]["kind"] == "schematic_pin" else \
                                self._docs.get(neighbor, {})
                            symbol = doc if doc["fields"]["kind"] == "schematic_symbol" else \
                                self._docs.get(neighbor, {})
                            pin_fields = pin.get("fields", {})
                            symbol_fields = symbol.get("fields", {})
                            if pin_fields.get("symbol_id") == symbol_fields.get("id"):
                                relationship = (
                                    "declared_pin"
                                    if pin_fields.get("membership_kind") == "declared_pin"
                                    else "logical_net_member")
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

    @staticmethod
    def _coordinate_domain(query: str, active_layer: str, exact_ids: set[str],
                           docs: dict[str, dict]) -> str:
        if re.search(r"\b(schematic|sheet|symbol|pin|wire|netlist)\b", query,
                     re.IGNORECASE):
            return "schematic"
        if active_layer or re.search(
                r"\b(pcb|board|physical|footprint|track|via|copper|layer)\b",
                query, re.IGNORECASE):
            return "board"
        if any(docs.get(uid, {}).get("fields", {}).get("kind") == "footprint"
               for uid in exact_ids):
            return "board"
        if any(docs.get(uid, {}).get("fields", {}).get("kind") in
               _SCHEMATIC_GEOMETRY_KINDS for uid in exact_ids):
            return "schematic"
        return "board"

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
                    "spatial_semantics": "axis_aligned_bounds_intersection_or_distance_only",
                    "geometry_relationship_semantics":
                        "pcb_coordinates_only; near_component_measures_anchor_position_to_footprint_bounds; region_member_means_axis_aligned_bounds_intersection",
                    "stats": {"index_state": "unavailable", "total_entities": 0,
                              "exact_match_count": 0, "lexical_match_count": 0,
                              "relationship_match_count": 0, "spatial_match_count": 0,
                              "near_component_match_count": 0,
                              "region_member_match_count": 0,
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

        query_box = _query_bounds(query)
        points = [] if query_box else _coordinates(query)
        nearby = bool(re.search(r"\b(near|around|nearby|within|radius|close\s+to|beside)\b",
                                query, re.IGNORECASE))
        coordinate_domain = self._coordinate_domain(query, active_layer, exact_ids,
                                                    self._docs)
        if nearby and not points:
            for uid in sorted(exact_ids):
                kind = self._docs.get(uid, {}).get("fields", {}).get("kind", "")
                if ((coordinate_domain == "board" and kind not in _BOARD_GEOMETRY_KINDS) or
                        (coordinate_domain == "schematic" and
                         kind not in _SCHEMATIC_GEOMETRY_KINDS)):
                    continue
                position = self._docs[uid]["fields"].get("position_mm")
                if position:
                    points.append((position["x"], position["y"]))
                if len(points) == 4:
                    break
        radius = _radius(query)
        spatial_scores: dict[str, float] = {}
        near_component_distances: dict[str, float] = {}
        anchor_ids = exact_ids if nearby and not _coordinates(query) else set()
        if query_box:
            spatial_scores.update({uid: distance for uid, distance in
                                   self._spatial_box(query_box)
                                   if self._docs[uid]["fields"]["kind"] in
                                   (_BOARD_GEOMETRY_KINDS if coordinate_domain == "board"
                                    else _SCHEMATIC_GEOMETRY_KINDS)})
        else:
            for point in points[:4]:
                for uid, distance in self._spatial(point, radius)[:limit * 2]:
                    kind = self._docs[uid]["fields"]["kind"]
                    if kind not in (_BOARD_GEOMETRY_KINDS if coordinate_domain == "board"
                                    else _SCHEMATIC_GEOMETRY_KINDS):
                        continue
                    if uid not in anchor_ids:
                        spatial_scores[uid] = min(spatial_scores.get(uid, math.inf), distance)

        if nearby and coordinate_domain == "board":
            for anchor_uid in sorted(exact_ids):
                anchor = self._docs.get(anchor_uid, {})
                if anchor.get("fields", {}).get("kind") != "footprint":
                    continue
                position = anchor["fields"].get("position_mm")
                if not position:
                    continue
                point = (position["x"], position["y"])
                for uid, distance in self._spatial(point, radius):
                    candidate = self._docs.get(uid, {})
                    if candidate.get("fields", {}).get("kind") != "footprint" or uid in exact_ids:
                        continue
                    related[uid].add("near_component")
                    near_component_distances[uid] = min(
                        near_component_distances.get(uid, math.inf), distance)
                    spatial_scores[uid] = min(spatial_scores.get(uid, math.inf), distance)

        region_intent = bool(re.search(
            r"\b(?:inside|within|intersect(?:s|ing)?|contained|members?|objects?|components?)\b",
            query, re.IGNORECASE))
        region_seeds = [uid for uid in sorted(exact_ids)
                        if self._docs.get(uid, {}).get("fields", {}).get("kind") ==
                        "placement_region"]
        if coordinate_domain == "board" and region_intent:
            for region_uid in region_seeds:
                bounds = self._docs[region_uid]["fields"].get("bounds_mm")
                if not bounds:
                    continue
                for uid, _distance in self._spatial_box(bounds):
                    if uid == region_uid or self._docs[uid]["fields"]["kind"] not in \
                            _BOARD_GEOMETRY_KINDS:
                        continue
                    related[uid].add("region_member")
                    spatial_scores[uid] = 0.0

        # A spatially selected object carries its actual live DRC/ERC records;
        # unrelated diagnostics elsewhere stay out.
        for uid, relations in self._relationship_neighbors(set(spatial_scores)).items():
            if (self._docs.get(uid, {}).get("fields", {}).get("kind") ==
                    "project_diagnostic" and "diagnostic_for" in relations):
                related[uid].add("diagnostic_for")

        geometry_anchor_ids = {
            uid for uid in exact_ids
            if ((nearby and coordinate_domain == "board" and
                 self._docs.get(uid, {}).get("fields", {}).get("kind") == "footprint") or
                uid in region_seeds)
        }
        scores: dict[str, tuple[float, str, str, float]] = {}
        for uid in exact_ids:
            scores[uid] = (1400.0 if uid in geometry_anchor_ids else 1000.0,
                           "exact", "", 0.0)
        for rank, (uid, score) in enumerate(lexical):
            candidate = (500.0 + score - rank * 0.001, "lexical", "", 0.0)
            if uid not in scores or candidate[0] > scores[uid][0]:
                scores[uid] = candidate
        for uid, relations in related.items():
            relation = sorted(relations)[0]
            geometry_relations = relations.intersection(
                {"near_component", "region_member"})
            if geometry_relations:
                relation = ("near_component" if "near_component" in geometry_relations
                            else "region_member")
                priority = 1250.0 if "near_component" in geometry_relations else 1200.0
                prior = scores.get(uid)
                if prior is None or prior[0] < priority:
                    scores[uid] = (priority, "relationship", relation,
                                   spatial_scores.get(uid, 0.0))
                elif not prior[2]:
                    scores[uid] = (prior[0], prior[1], relation, prior[3])
                continue
            if uid not in scores:
                scores[uid] = (100.0, "relationship", relation,
                               spatial_scores.get(uid, 0.0))
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
                               "description", "library_description", "footprint_name",
                               "lib_id", "sheet_path", "title", "text", "notes", "target",
                               "properties",
                               "pin_name", "pin_number", "electrical_type", "graphical_style",
                               "orientation", "identity_source", "symbol_unit", "locked", "visible",
                               "candidate_net_ids", "type", "net_id", "membership_kind", "layer_id",
                               "provenance", "source_group_id", "source_sheet_id", "member_count",
                               "members", "source_member_ids", "related_net_ids",
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
            if fields.get("kind") == "functional_block":
                item["source_revision"] = self._revision
            if relation:
                item["relationship"] = relation
                item["relationships"] = sorted(related.get(uid, ()))
            if mode == "spatial":
                item["distance_mm"] = round(distance, 4)
            elif "near_component" in related.get(uid, ()):
                item["distance_mm"] = round(near_component_distances.get(uid, distance), 4)
            encoded_size = len(json.dumps(item, ensure_ascii=False, separators=(",", ":")))
            if len(output) >= limit or used_chars + encoded_size > self.max_chars:
                continue
            used_chars += encoded_size
            output.append(item)

        stats.update({"exact_match_count": len(exact_ids),
                      "lexical_match_count": len(lexical_ids),
                      "relationship_match_count": len(related),
                      "spatial_match_count": len(spatial_scores),
                      "near_component_match_count": sum(
                          "near_component" in values for values in related.values()),
                      "region_member_match_count": sum(
                          "region_member" in values for values in related.values()),
                      "omitted_count": max(0, len(scores) - len(output)),
                      "total_entities": len(self._docs)})
        return {"available": True, "reason": "", "revision": self._revision,
                "entities": output, "characters": used_chars, "stats": stats,
                "relationship_semantics": "shared_net_association_only",
                "board_net_semantics":
                    "native_net_id_association_not_physical_continuity",
                "logical_net_semantics":
                    "schematic_membership_is_native_netlist_assignment_not_geometric_connectivity",
                "spatial_semantics": "axis_aligned_bounds_intersection_or_distance_only",
                "geometry_relationship_semantics":
                    "pcb_coordinates_only; near_component_measures_anchor_position_to_footprint_bounds; region_member_means_axis_aligned_bounds_intersection",
                "explicit_reference_semantics":
                    "serialized object references and declared schematic page membership only",
                "search_method": "exact_alias_bm25_relationship_spatial_geometry_diagnostics"}
