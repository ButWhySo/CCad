import os
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor, ConsoleSpanExporter

def setup_telemetry():
    # Set up basic TracerProvider
    provider = TracerProvider()
    
    # If Langfuse export is configured, we could add OTLP exporter
    # But for now, we'll setup a Console exporter to verify traces
    if os.environ.get("OTEL_EXPORTER_OTLP_ENDPOINT"):
        try:
            from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
            otlp_exporter = OTLPSpanExporter(endpoint=os.environ.get("OTEL_EXPORTER_OTLP_ENDPOINT"))
            provider.add_span_processor(BatchSpanProcessor(otlp_exporter))
        except ImportError:
            print("Warning: opentelemetry-exporter-otlp not installed. OTLP export disabled.", flush=True)
            
    # Add console exporter for local debugging
    if os.environ.get("CCAD_DEBUG_TELEMETRY") == "1":
        console_exporter = ConsoleSpanExporter()
        provider.add_span_processor(BatchSpanProcessor(console_exporter))
        
    trace.set_tracer_provider(provider)
    return trace.get_tracer("ccad_agent.orchestrator"), provider

tracer, trace_provider = setup_telemetry()

def trace_function(name: str):
    def decorator(func):
        def wrapper(*args, **kwargs):
            with tracer.start_as_current_span(name):
                return func(*args, **kwargs)
        return wrapper
    return decorator
