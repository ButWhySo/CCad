"""Keep the JSON-RPC dispatcher thin and protocol handlers independently analyzable."""

import ast
from pathlib import Path


SOURCE = (Path(__file__).resolve().parents[1] / "src" / "ccad_agent" /
          "orchestrator.py").read_text(encoding="utf-8")
TREE = ast.parse(SOURCE)
HANDLERS = {node.name: node for node in TREE.body if isinstance(node, ast.FunctionDef)}
assert "handle_human_message" in HANDLERS

human_dispatch = []
for node in ast.walk(TREE):
    if not isinstance(node, ast.If):
        continue
    test = node.test
    if (isinstance(test, ast.Compare) and isinstance(test.left, ast.Name)
            and test.left.id == "method" and test.comparators
            and isinstance(test.comparators[0], ast.Constant)
            and test.comparators[0].value == "human_message"):
        human_dispatch = node.body
        break
assert len(human_dispatch) == 1
dispatch_body = human_dispatch[0]
assert isinstance(dispatch_body, (ast.If, ast.Expr))
dispatch_calls = [
    node for node in ast.walk(dispatch_body)
    if isinstance(node, ast.Call)
    and isinstance(node.func, ast.Attribute)
    and isinstance(node.func.value, ast.Name)
    and node.func.value.id == "telemetry_runtime"
    and node.func.attr == "run_in_turn_context"
]
assert len(dispatch_calls) == 1
dispatch_call = dispatch_calls[0]
assert len(dispatch_call.args) == 2
assert isinstance(dispatch_call.args[0], ast.Name)
assert dispatch_call.args[0].id == "handle_human_message"

class LoopControl(ast.NodeVisitor):
    def __init__(self):
        self.loop_depth = 0
        self.invalid = []

    def visit_loop(self, node):
        self.loop_depth += 1
        self.generic_visit(node)
        self.loop_depth -= 1

    def visit_For(self, node):
        self.visit_loop(node)

    def visit_While(self, node):
        self.visit_loop(node)

    def visit_AsyncFor(self, node):
        self.visit_loop(node)

    def visit_Continue(self, node):
        if self.loop_depth == 0:
            self.invalid.append(("continue", node.lineno))

    def visit_Break(self, node):
        if self.loop_depth == 0:
            self.invalid.append(("break", node.lineno))


control = LoopControl()
control.visit(HANDLERS["handle_human_message"])
assert not control.invalid, control.invalid
print("PASS human-message handler is isolated from JSON-RPC dispatcher loop")
