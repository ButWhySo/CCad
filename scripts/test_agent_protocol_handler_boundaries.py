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
call = human_dispatch[0]
assert isinstance(call, ast.Expr) and isinstance(call.value, ast.Call)
assert isinstance(call.value.func, ast.Name)
assert call.value.func.id == "handle_human_message"
assert len(call.value.args) == 1

class LoopControl(ast.NodeVisitor):
    def __init__(self):
        self.loop_depth = 0
        self.invalid = []

    def visit_For(self, node):
        self.loop_depth += 1
        self.generic_visit(node)
        self.loop_depth -= 1

    visit_While = visit_For
    visit_AsyncFor = visit_For

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
