"""Metadata-only export boundary, including third-party callback spans."""
import re
from opentelemetry.sdk.trace import ReadableSpan
from opentelemetry.sdk.trace.export import SpanExporter
from opentelemetry.sdk.util.instrumentation import InstrumentationScope
from opentelemetry.trace import Status

_SECRET = re.compile(r"api[._-]?key|authorization|credential|password|secret|access[._-]?token|headers", re.I)
_VALUE = re.compile(r"(?:sk-|pk-lf-|Bearer\s+|Basic\s+|AIza)[A-Za-z0-9_./+=:-]+", re.I)
_CONTENT = re.compile(r"(?:^|[._])(input|output|prompt|messages?|arguments?|completion|file|path)(?:$|[._])", re.I)


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
        if _SECRET.search(key) or _CONTENT.search(key):
            continue
        if "metadata" in key and not isinstance(value, (int, float, bool)):
            if key.rsplit(".", 1)[-1] not in {"provider", "model", "workflow", "thread_id", "tool", "result"}:
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

    def export(self, spans):
        self.last_result = self.exporter.export(tuple(sanitize_span(s) for s in spans))
        return self.last_result

    def force_flush(self, timeout_millis=30000):
        return self.exporter.force_flush(timeout_millis)

    def shutdown(self):
        self.exporter.shutdown()
