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
assert safe.attributes["langfuse.observation.usage_details"] == '{"input":12,"output":3}'
assert safe.attributes["langfuse.observation.model.name"] == "selected-model"
assert "private" not in str(safe.attributes)
assert "secret" not in str(safe.attributes)
provider.shutdown()
print("trace privacy: payloads/events removed, usage/model preserved")
