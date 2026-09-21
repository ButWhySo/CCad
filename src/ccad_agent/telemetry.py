"""Runtime-configurable, redacted agent observability."""

from __future__ import annotations

import re
import threading
from typing import Any, Dict, List, Optional

from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider


_SENSITIVE_NAME = re.compile(
    r"(?:api[._-]?key|authorization|credential|password|secret|token|"
    r"otlp[._-]?headers?)", re.IGNORECASE)
_SENSITIVE_VALUE = re.compile(
    r"(?:sk-|pk-lf-|Bearer\s+|Basic\s+|AIza)[A-Za-z0-9_./+=:-]+", re.IGNORECASE)


def redact(value: Any, name: str = "") -> Any:
    """Remove credentials before an observability exporter can receive them."""
    if _SENSITIVE_NAME.search(name):
        return "[REDACTED]"
    if isinstance(value, str):
        return _SENSITIVE_VALUE.sub("[REDACTED]", value)
    if isinstance(value, dict):
        return {str(key): redact(item, str(key)) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [redact(item, name) for item in value]
    return value


class _NullSpanContext:
    def __enter__(self):
        return self

    def __exit__(self, *_: object) -> None:
        return None

    def set_attribute(self, *_: object) -> None:
        return None


class TelemetryRuntime:
    """Own one local OTel provider plus optional current Langfuse callback.

    OpenTelemetry permits replacing its global provider only once. This object
    never mutates that global provider, allowing a long-lived agent process to
    flush and replace its configured exporter after Settings changes.
    """

    def __init__(self) -> None:
        self._lock = threading.RLock()
        self._provider: Optional[TracerProvider] = None
        self._tracer = None
        self._langfuse_client = None
        self._langfuse_handler = None
        self._status: Dict[str, Any] = {
            "configured": False, "enabled": False,
            "exporter_initialized": False, "backend": "langfuse",
            "last_test": "not_run", "reason": "disabled",
            "secret_value_visible": False,
        }

    def _shutdown_locked(self) -> None:
        if self._langfuse_client is not None:
            try:
                self._langfuse_client.flush()
                self._langfuse_client.shutdown()
            except Exception:
                pass
        if self._provider is not None:
            try:
                self._provider.force_flush()
                self._provider.shutdown()
            except Exception:
                pass
        self._provider = None
        self._tracer = None
        self._langfuse_client = None
        self._langfuse_handler = None

    def configure(self, config: Dict[str, Any], secrets: Dict[str, str]) -> Dict[str, Any]:
        """Flush old exporters, then build the requested real Langfuse client."""
        enabled = bool(config.get("enabled", False))
        base_url = str(config.get("base_url", "")).strip()
        environment = str(config.get("environment", "development")).strip().lower()
        service_name = str(config.get("service_name", "ccad-agent")).strip()
        if not re.fullmatch(r"[a-z0-9][a-z0-9_-]*", environment or ""):
            environment = "development"
        if not service_name:
            service_name = "ccad-agent"
        public_key = str(secrets.get("public_key", "")).strip()
        secret_key = str(secrets.get("secret_key", "")).strip()
        with self._lock:
            self._shutdown_locked()
            self._status = {
                "configured": bool(public_key and secret_key), "enabled": enabled,
                "exporter_initialized": False, "backend": "langfuse",
                "last_test": "not_run",
                "reason": "disabled" if not enabled else "missing_credentials",
                "secret_value_visible": False,
            }
            if not enabled or not (public_key and secret_key):
                return self.status()
            try:
                # Current Langfuse SDK: v3+ handler path. Secrets pass only to
                # constructors, never config/events/attributes/logs.
                from langfuse import Langfuse
                from langfuse.langchain import CallbackHandler

                provider = TracerProvider(resource=Resource.create({
                    "service.name": service_name,
                    "ccad.observability.backend": "langfuse",
                }))
                self._provider = provider
                self._tracer = provider.get_tracer("ccad_agent.orchestrator")
                self._langfuse_client = Langfuse(
                    public_key=public_key, secret_key=secret_key,
                    base_url=base_url or None, environment=environment,
                    tracing_enabled=True,
                    mask=lambda *, data, **_: redact(data),
                    tracer_provider=provider,
                )
                self._langfuse_handler = CallbackHandler(public_key=public_key)
                self._status.update({"exporter_initialized": True, "reason": "ready"})
            except Exception as error:
                self._shutdown_locked()
                self._status.update({
                    "reason": "dependency_or_configuration_error",
                    "error_type": type(error).__name__,
                })
            return self.status()

    def status(self) -> Dict[str, Any]:
        with self._lock:
            return dict(self._status)

    def callbacks(self) -> List[Any]:
        with self._lock:
            return [self._langfuse_handler] if self._langfuse_handler is not None else []

    def start_span(self, name: str):
        with self._lock:
            return (self._tracer.start_as_current_span(name)
                    if self._tracer is not None else _NullSpanContext())

    def test_export(self) -> Dict[str, Any]:
        with self._lock:
            if not self._status.get("exporter_initialized"):
                self._status["last_test"] = "not_configured"
                return self.status()
            try:
                with self._tracer.start_as_current_span("ccad.observability.connection_test") as span:
                    span.set_attribute("ccad.observability.test", True)
                    span.set_attribute("ccad.observability.backend", "langfuse")
                self._provider.force_flush()
                self._langfuse_client.flush()
                self._status["last_test"] = "flushed"
            except Exception as error:
                self._status.update({"last_test": "failed", "reason": "export_failed",
                                     "error_type": type(error).__name__})
            return self.status()

    def shutdown(self) -> None:
        with self._lock:
            self._shutdown_locked()


runtime = TelemetryRuntime()


def trace_function(name: str):
    def decorator(func):
        def wrapper(*args, **kwargs):
            with runtime.start_span(name):
                return func(*args, **kwargs)
        return wrapper
    return decorator
