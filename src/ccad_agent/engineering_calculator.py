"""Bounded deterministic engineering calculations for the Agent tool surface.

Lengths are represented internally in millimetres, angles in radians, and
impedance in ohms. Expressions are parsed as a small arithmetic language; no
Python evaluation, imports, attribute access, or filesystem/process functions
are available.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation, localcontext
import math
import re


class CalculatorError(ValueError):
    """A safe, stable calculator failure category."""

    def __init__(self, category: str):
        self.category = category
        super().__init__(category)


@dataclass(frozen=True)
class Quantity:
    value: Decimal
    # Length, angle, impedance dimensions. Fractional exponents support sqrt.
    dimensions: tuple[Decimal, Decimal, Decimal]


@dataclass(frozen=True)
class CalculationResult:
    value: float
    unit: str


_ZERO = Decimal(0)
_ONE = Decimal(1)
_DIMENSIONLESS = (_ZERO, _ZERO, _ZERO)
_LENGTH = (_ONE, _ZERO, _ZERO)
_ANGLE = (_ZERO, _ONE, _ZERO)
_IMPEDANCE = (_ZERO, _ZERO, _ONE)
_PI = Decimal("3.1415926535897932384626433832795028841971693993751")
_MAX_MAGNITUDE = Decimal("1e18")
_MAX_EXPRESSION_CHARS = 512
_MAX_AST_NODES = 64
_MAX_AST_DEPTH = 12
_UNIT_PATTERN = re.compile(
    r"(?<![A-Za-z0-9_.])((?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*(kohm|mohm|ohm|mm|cm|nm|mil|mils|in|inch|inches|m|deg|degree|degrees|rad|radians)\b",
    re.IGNORECASE,
)


def _unit(name: str) -> Quantity:
    key = name.casefold()
    if key.startswith("unit_"):
        key = key[5:]
    units = {
        "mm": (Decimal(1), _LENGTH),
        "cm": (Decimal(10), _LENGTH),
        "nm": (Decimal("0.000001"), _LENGTH),
        "mil": (Decimal("0.0254"), _LENGTH),
        "mils": (Decimal("0.0254"), _LENGTH),
        "in": (Decimal("25.4"), _LENGTH),
        "inch": (Decimal("25.4"), _LENGTH),
        "inches": (Decimal("25.4"), _LENGTH),
        "m": (Decimal(1000), _LENGTH),
        "rad": (Decimal(1), _ANGLE),
        "radians": (Decimal(1), _ANGLE),
        "deg": (_PI / Decimal(180), _ANGLE),
        "degree": (_PI / Decimal(180), _ANGLE),
        "degrees": (_PI / Decimal(180), _ANGLE),
        "ohm": (Decimal(1), _IMPEDANCE),
        "mohm": (Decimal("0.001"), _IMPEDANCE),
        "kohm": (Decimal(1000), _IMPEDANCE),
    }
    try:
        scale, dimensions = units[key]
    except KeyError as error:
        raise CalculatorError("unknown_unit") from error
    return Quantity(scale, dimensions)


def _check(quantity: Quantity) -> Quantity:
    if (not quantity.value.is_finite() or abs(quantity.value) > _MAX_MAGNITUDE or
            any(not dimension.is_finite() or abs(dimension) > Decimal(8)
                for dimension in quantity.dimensions)):
        raise CalculatorError("numeric_limit_exceeded")
    return quantity


def _same_dimensions(left: Quantity, right: Quantity) -> None:
    if left.dimensions != right.dimensions:
        raise CalculatorError("incompatible_dimensions")


def _combine_dimensions(left: Quantity, right: Quantity, multiplier: Decimal) -> tuple[Decimal, Decimal, Decimal]:
    return (left.dimensions[0] + multiplier * right.dimensions[0],
            left.dimensions[1] + multiplier * right.dimensions[1],
            left.dimensions[2] + multiplier * right.dimensions[2])


def _normalise(expression: str) -> str:
    if not isinstance(expression, str) or not expression.strip():
        raise CalculatorError("expression_required")
    if len(expression) > _MAX_EXPRESSION_CHARS:
        raise CalculatorError("expression_too_long")
    if any(ord(char) < 32 for char in expression):
        raise CalculatorError("unsupported_expression")
    # Friendly EDA notation such as `5 cm` becomes an explicit multiplication.
    return _UNIT_PATTERN.sub(lambda match: f"({match.group(1)}*unit_{match.group(2).casefold()})",
                             expression.strip())


def _tree_depth(node: ast.AST, depth: int = 0) -> int:
    if depth > _MAX_AST_DEPTH:
        raise CalculatorError("expression_too_complex")
    return max((_tree_depth(child, depth + 1) for child in ast.iter_child_nodes(node)),
               default=depth)


def _evaluate(node: ast.AST) -> Quantity:
    if (isinstance(node, ast.Constant) and not isinstance(node.value, bool) and
            isinstance(node.value, (int, float))):
        try:
            value = Decimal(str(node.value))
        except InvalidOperation as error:
            raise CalculatorError("invalid_number") from error
        if not value.is_finite():
            raise CalculatorError("invalid_number")
        return _check(Quantity(value, _DIMENSIONLESS))
    if isinstance(node, ast.Name):
        return _unit(node.id)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
        value = _evaluate(node.operand)
        return _check(Quantity(value.value if isinstance(node.op, ast.UAdd)
                               else -value.value, value.dimensions))
    if isinstance(node, ast.BinOp):
        left, right = _evaluate(node.left), _evaluate(node.right)
        if isinstance(node.op, (ast.Add, ast.Sub)):
            _same_dimensions(left, right)
            value = left.value + right.value if isinstance(node.op, ast.Add) \
                else left.value - right.value
            return _check(Quantity(value, left.dimensions))
        if isinstance(node.op, (ast.Mult, ast.Div)):
            if isinstance(node.op, ast.Div) and right.value == 0:
                raise CalculatorError("division_by_zero")
            sign = Decimal(1) if isinstance(node.op, ast.Mult) else Decimal(-1)
            value = left.value * right.value if sign > 0 else left.value / right.value
            dimensions = _combine_dimensions(left, right, sign)
            return _check(Quantity(value, dimensions))
        if isinstance(node.op, ast.Pow):
            if right.dimensions != _DIMENSIONLESS or right.value != right.value.to_integral_value():
                raise CalculatorError("integer_dimensionless_exponent_required")
            if abs(right.value) > 12:
                raise CalculatorError("numeric_limit_exceeded")
            exponent = int(right.value)
            if left.value == 0 and exponent < 0:
                raise CalculatorError("division_by_zero")
            dimensions = (left.dimensions[0] * exponent,
                          left.dimensions[1] * exponent,
                          left.dimensions[2] * exponent)
            return _check(Quantity(left.value ** exponent, dimensions))
    if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and not node.keywords:
        name = node.func.id.casefold()
        values = [_evaluate(argument) for argument in node.args]
        return _evaluate_function(name, values)
    raise CalculatorError("unsupported_expression")


def _evaluate_function(name: str, values: list[Quantity]) -> Quantity:
    if name in {"sin", "cos", "tan"} and len(values) == 1:
        value = values[0]
        if value.dimensions != _ANGLE:
            raise CalculatorError("angle_required")
        try:
            result = Decimal(str(getattr(math, name)(float(value.value))))
        except (ValueError, OverflowError) as error:
            raise CalculatorError("invalid_trigonometric_input") from error
        return _check(Quantity(result, _DIMENSIONLESS))
    if name == "sqrt" and len(values) == 1:
        value = values[0]
        if value.value < 0:
            raise CalculatorError("negative_square_root")
        with localcontext() as context:
            context.prec = 40
            result = value.value.sqrt()
        dimensions = (value.dimensions[0] / 2, value.dimensions[1] / 2,
                      value.dimensions[2] / 2)
        return _check(Quantity(result, dimensions))
    if name == "abs" and len(values) == 1:
        return _check(Quantity(abs(values[0].value), values[0].dimensions))
    if name in {"min", "max"} and 1 <= len(values) <= 16:
        for value in values[1:]:
            _same_dimensions(values[0], value)
        chosen = (min if name == "min" else max)(values, key=lambda item: item.value)
        return chosen
    raise CalculatorError("unsupported_function_or_arity")


def _display(quantity: Quantity, requested_unit: str | None) -> CalculationResult:
    if requested_unit:
        unit = _unit(requested_unit)
        _same_dimensions(quantity, unit)
        value = quantity.value / unit.value
        label = requested_unit.casefold()
    elif quantity.dimensions == _LENGTH:
        value, label = quantity.value, "mm"
    elif quantity.dimensions == _ANGLE:
        value, label = quantity.value * Decimal(180) / _PI, "deg"
    elif quantity.dimensions == _IMPEDANCE:
        value, label = quantity.value, "ohm"
    elif quantity.dimensions == _DIMENSIONLESS:
        value, label = quantity.value, "1"
    else:
        label = "*".join(f"{name}^{dimension}" for name, dimension in
                         zip(("mm", "deg", "ohm"), quantity.dimensions) if dimension)
        value = quantity.value
    result = float(value)
    if not math.isfinite(result):
        raise CalculatorError("numeric_limit_exceeded")
    return CalculationResult(result, label)


def calculate(expression: str, output_unit: str | None = None) -> CalculationResult:
    """Evaluate bounded arithmetic with compatible physical dimensions."""
    try:
        tree = ast.parse(_normalise(expression), mode="eval")
    except (SyntaxError, ValueError, MemoryError) as error:
        raise CalculatorError("unsupported_expression") from error
    if sum(1 for _ in ast.walk(tree)) > _MAX_AST_NODES:
        raise CalculatorError("expression_too_complex")
    _tree_depth(tree)
    with localcontext() as context:
        context.prec = 40
        return _display(_evaluate(tree.body), output_unit)


def transform_point(*, x_mm: float, y_mm: float, rotation_deg: float = 0.0,
                    origin_x_mm: float = 0.0, origin_y_mm: float = 0.0,
                    translate_x_mm: float = 0.0, translate_y_mm: float = 0.0) -> dict:
    """Rotate a board point about an origin, then translate it; all lengths mm."""
    values = (x_mm, y_mm, rotation_deg, origin_x_mm, origin_y_mm,
              translate_x_mm, translate_y_mm)
    if any(not math.isfinite(value) or abs(value) > 1e9 for value in values):
        raise CalculatorError("invalid_coordinate")
    radians = math.radians(rotation_deg)
    cosine, sine = math.cos(radians), math.sin(radians)
    dx, dy = x_mm - origin_x_mm, y_mm - origin_y_mm
    return {"x_mm": origin_x_mm + dx * cosine - dy * sine + translate_x_mm,
            "y_mm": origin_y_mm + dx * sine + dy * cosine + translate_y_mm,
            "rotation_deg": rotation_deg % 360.0}


def microstrip_impedance(*, width_mm: float, substrate_height_mm: float,
                         dielectric_constant: float) -> dict:
    """Estimate zero-thickness microstrip Z0 with Hammerstad-Jensen equations."""
    values = (width_mm, substrate_height_mm, dielectric_constant)
    if any(not math.isfinite(value) or abs(value) > 1e6 for value in values):
        raise CalculatorError("invalid_geometry")
    if width_mm <= 0 or substrate_height_mm <= 0:
        raise CalculatorError("invalid_geometry")
    if not 1.0 < dielectric_constant <= 1000.0:
        raise CalculatorError("invalid_dielectric_constant")
    ratio = width_mm / substrate_height_mm
    if not math.isfinite(ratio) or not 1e-6 <= ratio <= 1e6:
        raise CalculatorError("geometry_outside_model_range")
    if ratio <= 1.0:
        epsilon_effective = ((dielectric_constant + 1.0) / 2.0 +
                             (dielectric_constant - 1.0) / 2.0 *
                             (1.0 / math.sqrt(1.0 + 12.0 / ratio) +
                              0.04 * (1.0 - ratio) ** 2))
        impedance = (60.0 / math.sqrt(epsilon_effective) *
                     math.log(8.0 / ratio + 0.25 * ratio))
    else:
        epsilon_effective = ((dielectric_constant + 1.0) / 2.0 +
                             (dielectric_constant - 1.0) / 2.0 /
                             math.sqrt(1.0 + 12.0 / ratio))
        impedance = (120.0 * math.pi /
                     (math.sqrt(epsilon_effective) *
                      (ratio + 1.393 + 0.667 * math.log(ratio + 1.444))))
    if not math.isfinite(impedance) or impedance <= 0:
        raise CalculatorError("calculation_failed")
    return {"value": impedance, "unit": "ohm", "effective_dielectric_constant": epsilon_effective,
            "model": "Hammerstad-Jensen zero-thickness microstrip",
            "assumptions": ["width and substrate height are in millimetres",
                            "conductor thickness and solder mask are ignored"]}


def build_engineering_tools() -> list:
    """Return provider-callable, read-only tools backed by these calculations."""
    from langchain_core.tools import StructuredTool
    from pydantic import Field, create_model

    calculate_args = create_model(
        "CCad_CalculateArgs",
        expression=(str, Field(..., min_length=1, max_length=_MAX_EXPRESSION_CHARS,
                               description="Arithmetic, e.g. '5 cm + 20 mm', 'sin(90 deg)', or 'sqrt(4 mm * 4 mm)'")),
        output_unit=(str | None, Field(None, max_length=12,
                                       description="Optional compatible result unit, e.g. mil, nm, deg, or ohm.")),
    )
    transform_args = create_model(
        "CCad_TransformPointArgs",
        x_mm=(float, Field(..., description="Input point X in millimetres.")),
        y_mm=(float, Field(..., description="Input point Y in millimetres.")),
        rotation_deg=(float, Field(0.0, description="Counterclockwise rotation in degrees.")),
        origin_x_mm=(float, Field(0.0, description="Rotation origin X in millimetres.")),
        origin_y_mm=(float, Field(0.0, description="Rotation origin Y in millimetres.")),
        translate_x_mm=(float, Field(0.0, description="Post-rotation X translation in millimetres.")),
        translate_y_mm=(float, Field(0.0, description="Post-rotation Y translation in millimetres.")),
    )
    impedance_args = create_model(
        "CCad_MicrostripImpedanceArgs",
        width_mm=(float, Field(..., gt=0, description="Trace width in millimetres.")),
        substrate_height_mm=(float, Field(..., gt=0, description="Dielectric height to reference plane in millimetres.")),
        dielectric_constant=(float, Field(..., gt=1, le=1000,
                                          description="Dimensionless substrate relative permittivity.")),
    )

    def invoke_calculate(expression: str, output_unit: str | None = None) -> str:
        try:
            result = calculate(expression, output_unit)
            return json_result({"ok": True, "value": result.value, "unit": result.unit,
                                "exact": False, "expression_chars": len(expression)})
        except CalculatorError as error:
            return json_result({"ok": False, "error": error.category})

    def invoke_transform(**kwargs) -> str:
        try:
            return json_result({"ok": True, **transform_point(**kwargs)})
        except CalculatorError as error:
            return json_result({"ok": False, "error": error.category})

    def invoke_impedance(**kwargs) -> str:
        try:
            return json_result({"ok": True, **microstrip_impedance(**kwargs)})
        except CalculatorError as error:
            return json_result({"ok": False, "error": error.category})

    def json_result(value: dict) -> str:
        import json
        return json.dumps(value, ensure_ascii=False, sort_keys=True, allow_nan=False)

    return [
        StructuredTool.from_function(
            invoke_calculate, name="ccad_calculate",
            description="Deterministic read-only dimensional arithmetic. Supports mm, cm, nm, mil, inches, m, deg, rad, and ohm; dimensional mismatch is rejected. Example: expression='5 cm + 20 mm', output_unit='mil'. No project changes.",
            args_schema=calculate_args),
        StructuredTool.from_function(
            invoke_transform, name="ccad_transform_point",
            description="Read-only coordinate transform: rotate an XY board point counterclockwise about an origin, then translate. Inputs and outputs use millimetres; rotation uses degrees.",
            args_schema=transform_args),
        StructuredTool.from_function(
            invoke_impedance, name="ccad_microstrip_impedance",
            description="Read-only Hammerstad-Jensen zero-thickness microstrip impedance estimate. Requires trace width, substrate height, and relative dielectric constant; reports assumptions, ignores copper thickness and solder mask.",
            args_schema=impedance_args),
    ]
