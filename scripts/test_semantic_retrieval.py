"""Local-only Ollama embedding and hybrid memory retrieval contracts."""

from __future__ import annotations

import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import sys
from threading import Thread
import tempfile

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from memory_manager import MemoryManager
from memory_store import MemoryStore
from semantic_retrieval import EmbeddingError, OllamaEmbeddingBackend


class OllamaTestHandler(BaseHTTPRequestHandler):
    @staticmethod
    def vector(value):
        text = value.casefold()
        if "regulator" in text or "step down" in text:
            return [1.0, 0.0, 0.0]
        if "ground" in text or "return" in text:
            return [0.0, 1.0, 0.0]
        return [0.0, 0.0, 1.0]

    def do_GET(self):
        if self.path != "/api/tags":
            self.send_error(404)
            return
        self._json({"models": [{"name": "test-embed:latest", "digest": "sha256:fixture-v1"}]})

    def do_POST(self):
        if self.path != "/api/embed":
            self.send_error(404)
            return
        body = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        values = body["input"]
        values = [values] if isinstance(values, str) else values
        vectors = [self.vector(value) for value in values]
        self._json({"model": body["model"], "embeddings": vectors})

    def _json(self, value):
        data = json.dumps(value).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *_):
        pass


class MemoryEmbeddingBackend:
    identity = "test-embed:sha256-fixture-v1"

    def __init__(self):
        self.calls = 0

    def embed_documents(self, texts):
        self.calls += 1
        return [OllamaTestHandler.vector(text) for text in texts]

    def embed_query(self, text):
        self.calls += 1
        return OllamaTestHandler.vector(text)


class FailingEmbeddingBackend:
    identity = "test-embed:raises-runtime-error"

    def embed_documents(self, texts):
        raise RuntimeError("internal error must not reach user-facing state")

    def embed_query(self, text):
        raise RuntimeError("internal error must not reach user-facing state")


server = ThreadingHTTPServer(("127.0.0.1", 0), OllamaTestHandler)
thread = Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    endpoint = f"http://127.0.0.1:{server.server_port}"
    backend = OllamaEmbeddingBackend(endpoint, "test-embed")
    assert backend.check_ready()["ready"] is True
    assert backend.model_version == "sha256:fixture-v1"
    assert backend.embed_documents(["switching regulator powers board"])[0] == [1.0, 0.0, 0.0]
    assert backend.embed_query("how to step down supply voltage") == [1.0, 0.0, 0.0]
    for bad_endpoint in ("https://127.0.0.1:11434", "http://example.com:11434",
                         "http://user:pass@127.0.0.1:11434",
                         "http://localhost:11434"):
        try:
            OllamaEmbeddingBackend(bad_endpoint, "test-embed")
        except EmbeddingError as error:
            assert error.category == "embedding_endpoint_not_local"
        else:
            raise AssertionError(f"unsafe endpoint accepted: {bad_endpoint}")

    with tempfile.TemporaryDirectory() as directory:
        manager = MemoryManager(MemoryStore(Path(directory) / "memory.json"),
                                thread_id="semantic-thread")
        manager.set_embedding_backend(MemoryEmbeddingBackend())
        manager.configure({"ltm": True, "semantic": {"enabled": True}})
        regulator = manager.add("switching regulator powers board", tier="ltm",
                                title="Power conversion")
        manager.add("keep ground return beside input", tier="ltm",
                    title="Ground routing")
        entries, diagnostics = manager.retrieve_with_metadata(
            "how to step down supply voltage", limit=4)
        assert entries and entries[0]["id"] == regulator["id"]
        assert diagnostics[0]["ranking_method"] == "hybrid_bm25_rrf_mmr"
        assert diagnostics[0]["channel_ranks"]["semantic"] == 1
        assert diagnostics[0]["bm25_score"] == 0
        assert manager.state()["semantic"]["ready"] is True
        backend_calls = manager._embedding_backend.calls
        manager.retrieve("how to step down supply voltage", limit=4)
        assert manager._embedding_backend.calls == backend_calls
        assert manager.embedding_cache_entries > 0
        manager.disable("ltm")
        assert manager.embedding_cache_entries == 0
        manager.enable("ltm")
        manager.configure({"ltm": True, "semantic": {
            "enabled": True, "backend": "ollama_local",
            "base_url": endpoint, "model": "new-model"}})
        assert manager.embedding_cache_entries == 0

    with tempfile.TemporaryDirectory() as directory:
        fallback = MemoryManager(MemoryStore(Path(directory) / "unexpected-error.json"))
        fallback.set_embedding_backend(FailingEmbeddingBackend())
        fallback.configure({"ltm": True, "semantic": {"enabled": True}})
        lexical_record = fallback.add("voltage converter powers input rail", tier="ltm")
        result = fallback.retrieve("voltage converter", limit=2)
        assert result and result[0]["id"] == lexical_record["id"]
        assert fallback.state()["semantic"]["status"] == "embedding_failed"
        assert "internal error" not in str(fallback.state()["semantic"])

    with tempfile.TemporaryDirectory() as directory:
        lexical_only = MemoryManager(MemoryStore(Path(directory) / "fallback.json"))
        lexical_only.configure({"ltm": True, "semantic": {
            "enabled": True, "backend": "ollama_local",
            "base_url": "http://127.0.0.1:1", "model": "missing"}})
        state = lexical_only.state()["semantic"]
        assert state["enabled"] is True and state["ready"] is False
        assert state["status"] == "service_unavailable"
        lexical_only.add("voltage converter power stage design", tier="ltm")
        assert lexical_only.retrieve("voltage converter")
finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=2)

print("PASS local Ollama protocol, loopback policy, hybrid paraphrase retrieval, cache and safe fallback")
