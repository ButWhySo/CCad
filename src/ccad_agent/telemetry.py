"""Runtime-configurable Langfuse tracing with a metadata-only export boundary."""
from __future__ import annotations

import base64
import hashlib
import threading
import time
from contextlib import contextmanager, nullcontext
from functools import wraps
from urllib.parse import urlsplit

from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import SpanExportResult
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from telemetry_privacy import MetadataOnlyExporter, redact


def _mask_callback_payload(*, data=None, **_):
    """Keep the SDK callback compatible while the exporter enforces privacy."""
    return redact(data)


class TelemetryRuntime:
    """One owner per agent child process; no global OTel provider replacement.

    The command loop serializes reconfiguration with graph execution. Langfuse
    4.7 caches resources by public key, including closed exporters. Reset that
    process-owned cache when replacing a configuration, not on status reads.
    """
    def __init__(self):
        self._lock = threading.RLock()
        self._provider = None
        self._langfuse_client = None
        self._langfuse_handler = None
        self._exporter = None
        self._fingerprint = None
        self._active_trace_id = ""
        self._active_span_id = ""
        self._last_trace_id = ""
        self._last_span_id = ""
        self._status = self._state(False, False, "disabled")

    @staticmethod
    def _state(enabled, configured, reason):
        return dict(enabled=enabled, configured=configured, reason=reason,
                    exporter_initialized=False, backend="langfuse",
                    last_test="not_run", connected=False,
                    data_policy="metadata_only", secret_value_visible=False)

    def _shutdown_locked(self):
        if self._langfuse_client is not None:
            # Pinned-SDK compatibility seam. CCad owns this child and is its
            # only Langfuse consumer. Reset shuts queues down before eviction;
            # keeping the cache would silently reuse old keys/host/provider.
            from langfuse._client.resource_manager import LangfuseResourceManager
            LangfuseResourceManager.reset()
        if self._provider is not None:
            self._provider.shutdown()
        self._provider = self._langfuse_client = self._langfuse_handler = None
        self._exporter = self._fingerprint = None

    def configure(self, config, secrets):
        enabled = bool(config.get("enabled", False))
        public_key = str(secrets.get("public_key", "")).strip()
        secret_key = str(secrets.get("secret_key", "")).strip()
        base_url = str(config.get("base_url") or "https://cloud.langfuse.com").strip().rstrip("/")
        environment = str(config.get("environment") or "development").strip()
        service = str(config.get("service_name") or "ccad-agent").strip()
        fingerprint = (enabled, base_url, environment, service,
                       hashlib.sha256(public_key.encode()).digest(),
                       hashlib.sha256(secret_key.encode()).digest())
        with self._lock:
            if fingerprint == self._fingerprint:
                return self.status()
            self._shutdown_locked()
            configured = bool(public_key and secret_key)
            self._status = self._state(enabled, configured, "disabled" if not enabled else "missing_credentials")
            if not enabled or not configured:
                self._fingerprint = fingerprint
                return self.status()
            url = urlsplit(base_url)
            if (url.username or url.password or url.query or url.fragment or not url.hostname
                    or (url.scheme != "https" and not (url.scheme == "http" and url.hostname in {"localhost", "127.0.0.1", "::1"}))):
                self._status["reason"] = "invalid_endpoint"
                return self.status()
            try:
                from langfuse import Langfuse
                from langfuse.langchain import CallbackHandler

                auth = base64.b64encode(f"{public_key}:{secret_key}".encode()).decode()
                self._exporter = MetadataOnlyExporter(OTLPSpanExporter(
                    endpoint=base_url + "/api/public/otel/v1/traces",
                    headers={"Authorization": "Basic " + auth}, timeout=5))
                # Avoid Resource.create(): auto-detected process/environment
                # attributes can contain local paths or deployment secrets.
                self._provider = TracerProvider(resource=Resource({"service.name": service}))
                self._langfuse_client = Langfuse(
                    public_key=public_key, secret_key=secret_key, base_url=base_url,
                    environment=environment, timeout=5, flush_interval=1,
                    tracer_provider=self._provider, span_exporter=self._exporter,
                    # Prevent callback payload/media processing before it queues.
                    # Exporter additionally protects raw OTel attrs/events.
                    mask=_mask_callback_payload, tracing_enabled=True)
                self._langfuse_handler = CallbackHandler(public_key=public_key)
                self._status.update(exporter_initialized=True, reason="ready",
                                    masking="metadata_only_export_boundary")
                self._fingerprint = fingerprint
            except Exception as error:
                self._shutdown_locked()
                self._status.update(reason="dependency_or_configuration_error", error_type=type(error).__name__)
            return self.status()

    def status(self):
        with self._lock:
            return dict(self._status)

    def callbacks(self):
        with self._lock:
            return [self._langfuse_handler] if self._langfuse_handler else []

    def current_trace(self):
        with self._lock:
            return {"trace_id": self._active_trace_id or self._last_trace_id or "unavailable",
                    "span_id": self._active_span_id or self._last_span_id or "unavailable"}

    def start_span(self, name):
        return self.observation(name)

    @contextmanager
    def observation(self, name, as_type="span", metadata=None, model=None):
        with self._lock:
            client = self._langfuse_client
        if client is None:
            yield None
            return
        with client.start_as_current_observation(name=name, as_type=as_type,
                metadata=redact(metadata or {}), model=model) as observation:
            with self._lock:
                self._active_trace_id = str(observation.trace_id)
                self._active_span_id = str(observation.id)
                self._last_trace_id = self._active_trace_id
                self._last_span_id = self._active_span_id
            try:
                yield observation
            finally:
                with self._lock:
                    self._active_trace_id = self._active_span_id = ""

    def session(self, thread_id):
        if self._langfuse_client is None:
            return nullcontext()
        from langfuse import propagate_attributes
        return propagate_attributes(session_id=str(thread_id), tags=["ccad"])

    def test_export(self):
        """Emit a real trace and fetch that exact ID; flushing alone is not proof."""
        with self._lock:
            if self._langfuse_client is None:
                self._status["last_test"] = "not_configured"
                return self.status()
            self._status.update(last_test="running", connected=False)
            try:
                with self.observation("ccad.observability.connection_test") as observation:
                    trace_id = observation.trace_id
                if not self._provider.force_flush(timeout_millis=6000):
                    raise TimeoutError("flush_timeout")
                if self._exporter.last_result != SpanExportResult.SUCCESS:
                    self._status.update(last_test="failed", reason="export_rejected")
                    return self.status()
                self._status.update(trace_id=trace_id, last_test="not_received", reason="readback_pending")
                for attempt in range(3):
                    try:
                        trace = self._langfuse_client.api.trace.get(trace_id,
                            request_options={"timeout_in_seconds": 5, "max_retries": 0})
                        if trace.id == trace_id:
                            self._status.update(last_test="verified", connected=True, reason="ready",
                                                observation_count=len(trace.observations or []))
                            break
                    except Exception as error:
                        self._status["error_type"] = type(error).__name__
                    if attempt < 2:
                        time.sleep(0.5)
            except Exception as error:
                self._status.update(last_test="failed", reason="export_or_readback_failed",
                                    error_type=type(error).__name__)
            return self.status()

    def shutdown(self):
        with self._lock:
            self._shutdown_locked()
            self._status.update(exporter_initialized=False, connected=False, reason="shutdown")


runtime = TelemetryRuntime()


def trace_function(name, as_type="span"):
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            with runtime.observation(name, as_type=as_type):
                return func(*args, **kwargs)
        return wrapper
    return decorator
