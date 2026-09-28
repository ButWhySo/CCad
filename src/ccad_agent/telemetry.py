"""Runtime-configurable Langfuse tracing with a metadata-only export boundary."""
from __future__ import annotations

import base64
import hashlib
import os
import sys
import threading
import time
from contextlib import ExitStack, contextmanager, nullcontext
from datetime import datetime, timedelta, timezone
from functools import wraps
from typing import Any, cast
from urllib.parse import urlsplit

from opentelemetry.sdk.resources import Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import SpanExportResult
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from telemetry_privacy import MetadataOnlyExporter, redact

LANGFUSE_INGESTION_VERSION_HEADER = "x-langfuse-ingestion-version"
LANGFUSE_READBACK_TIMEOUT_SECONDS = 20.0
LANGFUSE_TURN_READBACK_TIMEOUT_SECONDS = 2.0
LANGFUSE_READBACK_PAGE_SIZE = 100
LANGFUSE_METADATA_VALUE_LIMIT = 200


def _string_metadata(metadata):
    """Return Langfuse-v4-safe, redacted metadata with bounded string values."""
    if not isinstance(metadata, dict):
        return {}
    safe = cast(dict[str, Any], redact(metadata))
    result = {}
    for key, value in safe.items():
        if isinstance(value, bool):
            value = str(value).lower()
        elif isinstance(value, (str, int, float)):
            value = str(value)
        else:
            continue
        result[str(key)] = value[:LANGFUSE_METADATA_VALUE_LIMIT]
    return result


def _read_trace_observations(client, trace_id, timeout_seconds=LANGFUSE_READBACK_TIMEOUT_SECONDS):
    """Read all indexed v4 observations for one trace, bounded by a deadline."""
    deadline = time.monotonic() + timeout_seconds
    query_end = datetime.now(timezone.utc) + timedelta(minutes=1)
    query_start = query_end - timedelta(minutes=15)
    pause = 0.5
    last_error_type = ""
    while True:
        try:
            rows = []
            cursor = None
            while True:
                page = client.api.observations.get_many(
                    trace_id=trace_id,
                    limit=LANGFUSE_READBACK_PAGE_SIZE,
                    cursor=cursor,
                    from_start_time=query_start,
                    to_start_time=query_end,
                    request_options={"timeout_in_seconds": min(5, max(1, int(timeout_seconds))),
                                     "max_retries": 0},
                )
                rows.extend(item for item in page.data if item.trace_id == trace_id)
                cursor = page.meta.cursor
                if not cursor:
                    break
            if rows:
                return rows, ""
        except Exception as error:
            last_error_type = type(error).__name__
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return [], last_error_type
        time.sleep(min(pause, remaining))
        pause = min(pause * 2, 3.0)


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
        self._provider: Any = None
        self._langfuse_client: Any = None
        self._langfuse_handler: Any = None
        self._exporter: Any = None
        self._fingerprint = None
        self._active_trace_id = ""
        self._active_span_id = ""
        self._last_trace_id = ""
        self._last_span_id = ""
        self._observation_count = 0
        self._turn_scope = None
        self._turn_observation = None
        self._development_logging = os.environ.get("CCAD_TRACE_DEBUG", "").lower() in {"1", "true", "yes"}
        self._status = self._state(False, False, "disabled")

    @staticmethod
    def _state(enabled, configured, reason) -> dict[str, Any]:
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
        self._development_logging = (
            environment.lower() not in {"production", "prod"}
            or os.environ.get("CCAD_TRACE_DEBUG", "").lower() in {"1", "true", "yes"}
        )
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
                    or url.path not in {"", "/"}
                    or (url.scheme != "https" and not (url.scheme == "http" and url.hostname in {"localhost", "127.0.0.1", "::1"}))):
                self._status["reason"] = "invalid_endpoint"
                return self.status()
            try:
                from langfuse import Langfuse
                from langfuse.langchain import CallbackHandler

                auth = base64.b64encode(f"{public_key}:{secret_key}".encode()).decode()
                self._exporter = MetadataOnlyExporter(OTLPSpanExporter(
                    endpoint=base_url + "/api/public/otel/v1/traces",
                    headers={"Authorization": "Basic " + auth,
                             LANGFUSE_INGESTION_VERSION_HEADER: "4"}, timeout=5))
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

    def begin_turn(self):
        """Clear per-turn exporter and trace identity to prevent stale reports."""
        with self._lock:
            self._last_trace_id = ""
            self._last_span_id = ""
            self._observation_count = 0
            if self._exporter is not None:
                self._exporter.last_result = None
                self._exporter.last_span_count = 0

    def start_agent_turn(self, thread_id, metadata=None, *, input_data=None):
        """Open one active root so context and graph observations share a trace."""
        self.finish_agent_turn()
        self.begin_turn()
        with self._lock:
            if self._langfuse_client is None:
                return False
        scope = ExitStack()
        try:
            turn_id = str((metadata or {}).get("turn_id", ""))
            scope.enter_context(self.session(thread_id, turn_id=turn_id))
            root = scope.enter_context(self.observation(
                "agent.turn", "agent", metadata or {}, input_data=input_data))
        except Exception:
            scope.close()
            raise
        with self._lock:
            self._turn_scope = scope
            self._turn_observation = root
        return True

    def finish_agent_turn(self):
        """End the root once; callers can safely close at early-exit boundaries."""
        with self._lock:
            scope = self._turn_scope
            self._turn_scope = None
            observation = self._turn_observation
            self._turn_observation = None
        if scope is None:
            return False
        try:
            if observation is not None:
                observation.update(output={"terminal_state": "closed"})
        finally:
            scope.close()
        return True

    def flush_turn(self):
        """Flush one completed/failed turn and expose safe exporter diagnostics."""
        self.finish_agent_turn()
        with self._lock:
            error_type = ""
            provider = self._provider
            exporter = self._exporter
            trace_id = self._last_trace_id or "unavailable"
            if provider is None or exporter is None:
                result = "disabled"
                span_count = 0
            elif not self._development_logging:
                # The SDK's background batch processor owns production delivery.
                # Synchronous per-turn flushing is enabled in development so
                # developers can correlate a prompt with a concrete export result.
                result = "queued"
                span_count = int(exporter.last_span_count)
            else:
                try:
                    flushed = bool(provider.force_flush(timeout_millis=6000))
                    export_result = exporter.last_result
                    span_count = int(exporter.last_span_count)
                    if not flushed:
                        result = "flush_timeout"
                    elif export_result == SpanExportResult.SUCCESS:
                        result = "success"
                        client = self._langfuse_client
                        if client is not None and trace_id != "unavailable":
                            # A successful OTLP response only proves the
                            # collector accepted the batch. In development,
                            # verify this exact turn through the v4 observation API.
                            result = "not_received"
                            observations, error_type = _read_trace_observations(
                                client, trace_id, LANGFUSE_TURN_READBACK_TIMEOUT_SECONDS)
                            if observations:
                                result = "verified"
                                self._observation_count = len(observations)
                                error_type = ""
                    elif export_result is None:
                        result = "no_spans_exported"
                    else:
                        result = "export_failed"
                except Exception as error:
                    span_count = 0
                    result = "flush_error"
                    error_type = type(error).__name__
            self._status.update(last_export=result, trace_id=trace_id,
                                exported_span_count=span_count,
                                observation_count=self._observation_count,
                                last_export_ok=result in {"success", "verified"})
            if result in {"success", "verified"}:
                self._status.pop("last_export_error_type", None)
            elif error_type:
                self._status["last_export_error_type"] = error_type
            if self._development_logging:
                error_suffix = f" error_type={error_type}" if error_type else ""
                print(f"[ccad-otel] turn_flush trace_id={trace_id} result={result} "
                      f"spans={span_count}{error_suffix}", file=sys.stderr, flush=True)
            return self.status()

    def start_span(self, name):
        return self.observation(name)

    @contextmanager
    def observation(self, name, as_type="span", metadata=None, model=None,
                    input_data=None):
        with self._lock:
            client = self._langfuse_client
        if client is None:
            yield None
            return
        with client.start_as_current_observation(name=name, as_type=as_type,
                metadata=_string_metadata(metadata or {}), model=model,
                input=redact(input_data) if input_data is not None else None) as observation:
            if observation is None:
                raise RuntimeError("observation_not_created")
            observation = cast(Any, observation)
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

    def session(self, thread_id, *, turn_id=""):
        if self._langfuse_client is None:
            return nullcontext()
        from langfuse import propagate_attributes
        attributes = {"session_id": str(thread_id), "tags": ["ccad"]}
        if turn_id:
            attributes.update(
                trace_name="ccad.agent.turn",
                metadata={"ccad_turn_id": str(turn_id)},
            )
        return propagate_attributes(**attributes)

    def test_export(self):
        """Emit a real trace and fetch that exact ID; flushing alone is not proof."""
        with self._lock:
            if self._langfuse_client is None:
                self._status["last_test"] = "not_configured"
                return self.status()
            self._status.update(last_test="running", connected=False)
            try:
                with self.observation("ccad.observability.connection_test") as observation:
                    if observation is None:
                        raise RuntimeError("observation_not_created")
                    trace_id = observation.trace_id
                if not self._provider.force_flush(timeout_millis=6000):
                    raise TimeoutError("flush_timeout")
                if self._exporter.last_result != SpanExportResult.SUCCESS:
                    self._status.update(last_test="failed", reason="export_rejected")
                    return self.status()
                self._status.update(trace_id=trace_id, last_test="not_received", reason="readback_pending")
                observations, error_type = _read_trace_observations(self._langfuse_client, trace_id)
                if observations:
                    self._status.update(last_test="verified", last_export="verified",
                                        exported_span_count=int(self._exporter.last_span_count),
                                        last_export_ok=True, connected=True, reason="ready",
                                        observation_count=len(observations))
                elif error_type:
                    self._status["error_type"] = error_type
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
