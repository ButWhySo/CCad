"""Exercise the real OTel span representation without sending telemetry."""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "src" / "ccad_agent"))
from opentelemetry.sdk.trace import TracerProvider
from telemetry_privacy import sanitize_span

provider = TracerProvider()
span = provider.get_tracer("test").start_span("router")
span.set_attribute("langfuse.observation.input", "private board content")
span.set_attribute("langfuse.observation.output", "private conversation")
span.set_attribute("langfuse.observation.usage_details", '{"input":12,"output":3}')
span.set_attribute("langfuse.observation.model.name", "selected-model")
span.set_attribute("langfuse.observation.metadata.secret_key", "secret")
span.add_event("exception", {"exception.message": "private password"})
span.end()
safe = sanitize_span(span._readable_span())
assert not safe.events
assert "langfuse.observation.input" not in safe.attributes
assert "langfuse.observation.output" not in safe.attributes
assert safe.attributes["langfuse.observation.usage_details"] == '{"input":12,"output":3}'
assert safe.attributes["langfuse.observation.model.name"] == "selected-model"
assert "private" not in str(safe.attributes)
assert "secret" not in str(safe.attributes)

root = provider.get_tracer("test").start_span("agent.turn")
root.set_attribute("langfuse.observation.input", '{"prompt_sha256":"' + ("a" * 64) + '","prompt_chars":12}')
root.set_attribute("langfuse.observation.output", '{"terminal_state":"closed"}')
root.set_attribute("langfuse.observation.metadata.ccad_turn_id", "turn-contract")
root.end()
safe_root = sanitize_span(root._readable_span())
assert safe_root.attributes["langfuse.observation.input"] == (
    '{"prompt_chars":12,"prompt_sha256":"' + ("a" * 64) + '"}')
assert safe_root.attributes["langfuse.observation.output"] == '{"terminal_state":"closed"}'
assert safe_root.attributes["langfuse.observation.metadata.ccad_turn_id"] == "turn-contract"

unsafe_root = provider.get_tracer("test").start_span("agent.turn")
unsafe_root.set_attribute("langfuse.observation.input", '{"prompt":"raw user text"}')
unsafe_root.set_attribute("langfuse.observation.output", '{"answer":"private response"}')
unsafe_root.end()
safe_unsafe_root = sanitize_span(unsafe_root._readable_span())
assert "langfuse.observation.input" not in safe_unsafe_root.attributes
assert "langfuse.observation.output" not in safe_unsafe_root.attributes
provider.shutdown()
print("trace privacy: raw payloads/events removed; validated root digest/count preserved")
