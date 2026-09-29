"""Metadata-only export boundary, including third-party callback spans."""
import json
import re
from opentelemetry.sdk.trace import ReadableSpan
from opentelemetry.sdk.trace.export import SpanExporter
from opentelemetry.sdk.util.instrumentation import InstrumentationScope
from opentelemetry.trace import Status

_SECRET = re.compile(r"api[._-]?key|authorization|credential|password|secret|access[._-]?token|headers", re.I)
_VALUE = re.compile(r"(?:sk-|pk-lf-|Bearer\s+|Basic\s+|AIza)[A-Za-z0-9_./+=:-]+", re.I)
_CONTENT = re.compile(r"(?:^|[._])(input|output|prompt|messages?|arguments?|completion|file|path)(?:$|[._])", re.I)
_ROOT_IO_KEYS = {
    "langfuse.observation.input",
    "langfuse.observation.output",
}


def _safe_root_io(span, key, value):
    """Allow only the content-free root contract required by Langfuse v4."""
    if span.name != "agent.turn" or key not in _ROOT_IO_KEYS or not isinstance(value, str):
        return None
    try:
        payload = json.loads(value)
    except (TypeError, ValueError):
        return None
    if key == "langfuse.observation.input":
        if (not isinstance(payload, dict)
                or set(payload) != {"prompt_sha256", "prompt_chars"}
                or not isinstance(payload.get("prompt_sha256"), str)
                or not re.fullmatch(r"[0-9a-f]{64}", payload["prompt_sha256"])
                or not isinstance(payload.get("prompt_chars"), int)
                or isinstance(payload["prompt_chars"], bool)
                or not 0 <= payload["prompt_chars"] <= 10_000_000):
            return None
        safe = {"prompt_sha256": payload["prompt_sha256"],
                "prompt_chars": payload["prompt_chars"]}
    else:
        if not isinstance(payload, dict) or payload != {"terminal_state": "closed"}:
            return None
        safe = {"terminal_state": "closed"}
    return json.dumps(safe, sort_keys=True, separators=(",", ":"))


def redact(value, name=""):
    if _SECRET.search(name):
        return "[REDACTED]"
    if isinstance(value, str):
        return _VALUE.sub("[REDACTED]", value)
    if isinstance(value, dict):
        return {str(k): redact(v, str(k)) for k, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [redact(v) for v in value]
    return value


def sanitize_span(span):
    attributes = {}
    for key, value in (span.attributes or {}).items():
        # Numeric counters, usage JSON and model identity survive; arbitrary
        # callback inputs, outputs, exception text and filesystem paths do not.
        if _SECRET.search(key):
            continue
        if _CONTENT.search(key):
            safe_root_value = _safe_root_io(span, key, value)
            if safe_root_value is None:
                continue
            attributes[key] = safe_root_value
            continue
        if "metadata" in key and not isinstance(value, (int, float, bool)):
            if key.rsplit(".", 1)[-1] not in {
                    "provider", "model", "workflow", "thread_id", "tool", "result",
                    "ccad_turn_id"}:
                continue
        attributes[key] = redact(value)
    scope = span.instrumentation_scope
    return ReadableSpan(
        name=redact(span.name), context=span.context, parent=span.parent,
        resource=span.resource, attributes=attributes, events=(), links=(),
        kind=span.kind, status=Status(span.status.status_code),
        start_time=span.start_time, end_time=span.end_time,
        instrumentation_scope=InstrumentationScope(scope.name, scope.version) if scope else None,
    )


class MetadataOnlyExporter(SpanExporter):
    """Public OTel exporter interface; no SDK-private span mutation."""
    def __init__(self, exporter):
        self.exporter = exporter
        self.last_result = None
        self.last_span_count = 0

    def export(self, spans):
        safe_spans = tuple(sanitize_span(s) for s in spans)
        self.last_span_count += len(safe_spans)
        self.last_result = self.exporter.export(safe_spans)
        return self.last_result

    def force_flush(self, timeout_millis=30000):
        return self.exporter.force_flush(timeout_millis)

    def shutdown(self):
        self.exporter.shutdown()
