"""ACK follows pending-call and checkpoint correlation validation."""

import ast
from pathlib import Path


source = (Path(__file__).parents[1] / "src" / "ccad_agent" / "orchestrator.py").read_text(
    encoding="utf-8")
tree = ast.parse(source)


def compare_equals(node, name, value):
    return (isinstance(node, ast.Compare) and isinstance(node.left, ast.Name)
            and node.left.id == name and any(
                isinstance(operator, ast.Eq) and isinstance(comparator, ast.Constant)
                and comparator.value == value
                for operator, comparator in zip(node.ops, node.comparators)))


def emits_method(statement, method):
    for node in ast.walk(statement):
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Name):
            continue
        if node.func.id != "emit" or not node.args or not isinstance(node.args[0], ast.Dict):
            continue
        fields = {key.value: value.value for key, value in
                  zip(node.args[0].keys, node.args[0].values)
                  if isinstance(key, ast.Constant) and isinstance(value, ast.Constant)}
        if fields.get("method") == method:
            return True
    return False


def contains_attribute_call(statement, attribute):
    return any(isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
               and node.func.attr == attribute for node in ast.walk(statement))


tool_result = next(node for node in ast.walk(tree)
                   if isinstance(node, ast.If)
                   and compare_equals(node.test, "method", "tool_result"))
checkpoint = next(node for node in ast.walk(tool_result)
                  if isinstance(node, ast.If)
                  and "checkpoint_saver is not None" in ast.unparse(node.test)
                  and "executor is not None" in ast.unparse(node.test))
checkpoint_ack_index = next(index for index, statement in enumerate(checkpoint.body)
                            if emits_method(statement, "tool_result_ack"))
validation_guards = [statement for statement in checkpoint.body
                     if isinstance(statement, ast.If)
                     and any(value in ast.unparse(statement.test)
                             for value in ("snapshot.next", "expected_call_id"))]
assert len(validation_guards) >= 2
assert all(checkpoint.body.index(guard) < checkpoint_ack_index
           for guard in validation_guards)
assert all(guard.body and isinstance(guard.body[-1], ast.Continue)
           for guard in validation_guards)

fallback = checkpoint.orelse
fallback_ack_index = next(index for index, statement in enumerate(fallback)
                          if emits_method(statement, "tool_result_ack"))
queue_delivery_index = next(index for index, statement in enumerate(fallback)
                            if contains_attribute_call(statement, "put"))
assert fallback_ack_index < queue_delivery_index
ack_calls = [node for node in ast.walk(tool_result)
             if isinstance(node, ast.Call) and isinstance(node.func, ast.Name)
             and node.func.id == "emit" and node.args
             and isinstance(node.args[0], ast.Dict)
             and any(isinstance(key, ast.Constant) and key.value == "method"
                     and isinstance(value, ast.Constant)
                     and value.value == "tool_result_ack"
                     for key, value in zip(node.args[0].keys, node.args[0].values))]
assert len(ack_calls) == 2
print("PASS broker ACK follows correlation validation and precedes delivery; no network")
