"""No-network proof: runtime Agent methods remain discoverable."""

import ast
import sys
from pathlib import Path


root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "src" / "ccad_agent"))
from method_catalog import orchestrator_method_catalog  # noqa: E402

source = (root / "src" / "ccad_agent" / "orchestrator.py").read_text(encoding="utf-8")
tree = ast.parse(source)


def is_main_guard(node):
    return (isinstance(node, ast.If) and isinstance(node.test, ast.Compare)
            and isinstance(node.test.left, ast.Name)
            and node.test.left.id == "__name__"
            and any(isinstance(value, ast.Constant) and value.value == "__main__"
                    for value in node.test.comparators))


main_guard = next(node for node in tree.body if is_main_guard(node))
provider_handler = next(node for node in tree.body
                        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
                        and node.name == "handle_provider_and_state_request")


def method_values(scope):
    values = set()
    for node in ast.walk(scope):
        if not isinstance(node, ast.Compare) or not isinstance(node.left, ast.Name):
            continue
        if node.left.id != "method":
            continue
        for operator, comparator in zip(node.ops, node.comparators):
            candidates = ([comparator] if isinstance(operator, ast.Eq) else
                          comparator.elts if isinstance(operator, ast.In) and
                          isinstance(comparator, (ast.Tuple, ast.List, ast.Set)) else [])
            values.update(item.value for item in candidates
                          if isinstance(item, ast.Constant) and isinstance(item.value, str))
    return values


runtime = method_values(main_guard) | method_values(provider_handler)
catalog = {entry["name"] for entry in orchestrator_method_catalog()["methods"]}
assert runtime - catalog == set(), sorted(runtime - catalog)
assert catalog - runtime == set(), sorted(catalog - runtime)
print("PASS runtime Agent methods remain discoverable; no network")
