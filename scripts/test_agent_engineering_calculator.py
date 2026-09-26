"""Contract tests for bounded, deterministic engineering calculator tools."""

import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
AGENT = ROOT / "src" / "ccad_agent"
sys.path.insert(0, str(AGENT))
os.environ["CCAD_AGENT_DEFER_PROVIDER_INIT"] = "1"

from engineering_calculator import CalculatorError, calculate, microstrip_impedance, transform_point


def close(actual, expected, tolerance=1e-9):
    assert abs(actual - expected) <= tolerance, (actual, expected)


def test_dimensioned_arithmetic_and_conversion():
    close(calculate("5 cm + 20 mm").value, 70.0)
    close(calculate("1 in / 1000 mil").value, 1.0)
    close(calculate("1000000 nm").value, 1.0)
    close(calculate("sqrt(4 mm * 4 mm)").value, 4.0)
    assert calculate("5 cm + 20 mm").unit == "mm"
    assert calculate("90 deg").unit == "deg"


def test_trigonometry_uses_explicit_angle_units():
    close(calculate("sin(90 deg)").value, 1.0)
    close(calculate("cos(0 rad)").value, 1.0)
    with_error("sin(90)", "angle_required")


def test_dimension_mismatch_and_unsafe_syntax_fail_closed():
    for expression, category in (("1 mm + 1 deg", "incompatible_dimensions"),
                                 ("1 / 0", "division_by_zero"),
                                 ("__import__('os').system('whoami')", "unsupported_expression"),
                                 ("2 ** 1000", "numeric_limit_exceeded"),
                                 ("1e999999 mm", "invalid_number")):
        with_error(expression, category)


def test_coordinate_transform_rotates_about_given_origin_then_translates():
    result = transform_point(x_mm=10, y_mm=0, origin_x_mm=0, origin_y_mm=0,
                             rotation_deg=90, translate_x_mm=5, translate_y_mm=-2)
    close(result["x_mm"], 5.0)
    close(result["y_mm"], 8.0)
    close(result["rotation_deg"], 90.0)


def test_microstrip_impedance_requires_valid_physical_inputs():
    result = microstrip_impedance(width_mm=0.3, substrate_height_mm=0.2,
                                  dielectric_constant=4.2)
    assert result["unit"] == "ohm"
    assert 40.0 < result["value"] < 100.0
    assert result["model"] == "Hammerstad-Jensen zero-thickness microstrip"
    for values, category in ((dict(width_mm=0, substrate_height_mm=0.2,
                                   dielectric_constant=4.2), "invalid_geometry"),
                             (dict(width_mm=0.3, substrate_height_mm=0.2,
                                   dielectric_constant=1), "invalid_dielectric_constant")):
        try:
            microstrip_impedance(**values)
        except CalculatorError as error:
            assert error.category == category
        else:
            raise AssertionError("invalid microstrip inputs must fail")


def test_orchestrator_discloses_and_executes_read_only_calculators():
    spec = importlib.util.spec_from_file_location(
        "ccad_engineering_calculator_orchestrator", AGENT / "orchestrator.py")
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    catalog = [{"method": "project.context", "description": "Inspect project.",
                "read_only": True,
                "inputSchema": {"type": "object", "properties": {}, "required": []}}]
    state = module.install_native_tool_catalog(catalog)
    names = {tool.name for tool in module.agent_tools}
    assert {"ccad_calculate", "ccad_transform_point",
            "ccad_microstrip_impedance", "ccad_search_memory"} <= names
    assert state["tool_count"] == state["native_tool_count"] + 4
    assert state["local_tool_count"] == 4
    calculator = next(tool for tool in module.agent_tools if tool.name == "ccad_calculate")
    result = json.loads(calculator.invoke({"expression": "5 cm + 20 mm"}))
    assert result == {"ok": True, "value": 70.0, "unit": "mm",
                      "exact": False, "expression_chars": len("5 cm + 20 mm")}
    converted = json.loads(calculator.invoke({"expression": "5 cm + 20 mm",
                                              "output_unit": "mil"}))
    close(converted["value"], 2755.9055118110236)
    assert converted["unit"] == "mil"
    invalid = json.loads(calculator.invoke({"expression": "1 mm + 1 deg"}))
    assert invalid == {"ok": False, "error": "incompatible_dimensions"}
    transform = next(tool for tool in module.agent_tools
                     if tool.name == "ccad_transform_point")
    transformed = json.loads(transform.invoke({"x_mm": 10, "y_mm": 0,
                                                "rotation_deg": 90,
                                                "translate_x_mm": 5,
                                                "translate_y_mm": -2}))
    close(transformed["x_mm"], 5.0)
    close(transformed["y_mm"], 8.0)
    impedance = next(tool for tool in module.agent_tools
                     if tool.name == "ccad_microstrip_impedance")
    impedance_result = json.loads(impedance.invoke({"width_mm": 0.3,
                                                    "substrate_height_mm": 0.2,
                                                    "dielectric_constant": 4.2}))
    assert impedance_result["ok"] and impedance_result["unit"] == "ohm"
    assert "conductor thickness" in " ".join(impedance_result["assumptions"])
    schema = calculator.args_schema.model_json_schema()
    assert schema["required"] == ["expression"]
    assert "expression" in schema["properties"]
    with tempfile.TemporaryDirectory():
        assert not list(ROOT.glob("ccad_calculator_*.json"))


def with_error(expression, category):
    try:
        calculate(expression)
    except CalculatorError as error:
        assert error.category == category, (expression, error.category)
    else:
        raise AssertionError(f"expected {category} for {expression!r}")


if __name__ == "__main__":
    for test in (test_dimensioned_arithmetic_and_conversion,
                 test_trigonometry_uses_explicit_angle_units,
                 test_dimension_mismatch_and_unsafe_syntax_fail_closed,
                 test_coordinate_transform_rotates_about_given_origin_then_translates,
                 test_microstrip_impedance_requires_valid_physical_inputs,
                 test_orchestrator_discloses_and_executes_read_only_calculators):
        test()
    print("PASS deterministic engineering calculator contracts; no provider, project mutation, or filesystem output")
