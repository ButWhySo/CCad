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
    requests = []
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
        self._json({"models": [
            {"name": "test-embed:latest", "digest": "sha256:fixture-v1"},
            {"name": "embeddinggemma:latest", "digest": "sha256:gemma-fixture"},
            {"name": "nomic-embed-text:latest", "digest": "sha256:nomic-fixture"},
        ]})

    def do_POST(self):
        if self.path != "/api/embed":
            self.send_error(404)
            return
        body = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self.requests.append(body)
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

    def log_message(self, format: str, *args: object) -> None:
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
    gemma_backend = OllamaEmbeddingBackend(endpoint, "embeddinggemma")
    assert gemma_backend.check_ready()["ready"] is True
    similarity_query = "A switching regulator converts an input rail to stable output voltage."
    gemma_backend.embed_similarity_query(similarity_query)
    assert OllamaTestHandler.requests[-1]["input"] == [
        "task: sentence similarity | query: " + similarity_query]
    similarity_documents = ["The power rail uses a 0.25 mm track."]
    gemma_backend.embed_similarity_documents(similarity_documents)
    assert OllamaTestHandler.requests[-1]["input"] == [
        "task: sentence similarity | query: " + similarity_documents[0]]
    nomic_backend = OllamaEmbeddingBackend(endpoint, "nomic-embed-text")
    assert nomic_backend.check_ready()["ready"] is True
    nomic_backend.embed_similarity_query(similarity_query)
    assert OllamaTestHandler.requests[-1]["input"] == ["clustering: " + similarity_query]
    nomic_backend.embed_similarity_documents(similarity_documents)
    assert OllamaTestHandler.requests[-1]["input"] == [
        "clustering: " + similarity_documents[0]]
    ordinary_backend = OllamaEmbeddingBackend(endpoint, "test-embed")
    ordinary_backend.embed_similarity_query(similarity_query)
    assert OllamaTestHandler.requests[-1]["input"] == [similarity_query]
    with tempfile.TemporaryDirectory() as directory:
        ollama_manager = MemoryManager(
            MemoryStore(Path(directory) / "ollama-semantic-dedupe.json"),
            thread_id="semantic-thread")
        ollama_manager.configure({"ltm": True, "semantic": {
            "enabled": True, "model": "embeddinggemma", "base_url": endpoint}})
        ollama_manager.add(
            "A switching regulator powers the supply rail of the board",
            tier="ltm", scope="conversation")
        try:
            ollama_manager.add(
                "A step down regulator powers the board supply rail",
                tier="ltm", scope="conversation")
        except ValueError as error:
            assert "semantic similarity" in str(error)
        else:
            raise AssertionError("Ollama-backed semantic duplicate was accepted")
        assert "task: sentence similarity | query: " in \
            OllamaTestHandler.requests[-1]["input"][0]
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
        embedding_backend = MemoryEmbeddingBackend()
        manager.set_embedding_backend(embedding_backend)
        manager.configure({"ltm": True, "semantic": {"enabled": True}})
        regulator = manager.add("switching regulator powers board", tier="ltm",
                                title="Power conversion")
        assert regulator is not None
        manager.add("keep ground return beside input", tier="ltm",
                    title="Ground routing")
        entries, diagnostics = manager.retrieve_with_metadata(
            "how to step down supply voltage", limit=4)
        assert entries and entries[0]["id"] == regulator["id"]
        assert diagnostics[0]["ranking_method"] == "hybrid_bm25_rrf_mmr"
        assert diagnostics[0]["channel_ranks"]["semantic"] == 1
        assert diagnostics[0]["bm25_score"] == 0
        assert manager.state()["semantic"]["ready"] is True
        backend_calls = embedding_backend.calls
        manager.retrieve("how to step down supply voltage", limit=4)
        assert embedding_backend.calls == backend_calls
        assert manager.embedding_cache_entries > 0
        manager.disable("ltm")
        assert manager.embedding_cache_entries == 0
        manager.enable("ltm")
        manager.configure({"ltm": True, "semantic": {
            "enabled": True, "backend": "ollama_local",
            "base_url": endpoint, "model": "new-model"}})
        assert manager.embedding_cache_entries == 0

    with tempfile.TemporaryDirectory() as directory:
        manager = MemoryManager(MemoryStore(Path(directory) / "semantic-dedupe.json"),
                                thread_id="semantic-thread", project_id="project-a")
        embedding_backend = MemoryEmbeddingBackend()
        manager.set_embedding_backend(embedding_backend)
        manager.configure({"ltm": True, "semantic": {"enabled": True}})
        source = manager.add(
            "A switching regulator converts the input rail into stable output voltage",
            tier="ltm", scope="conversation", title="Power conversion")
        assert source is not None
        backend_calls = embedding_backend.calls
        try:
            manager.add("api_key=not-a-real-secret", tier="ltm",
                        scope="conversation")
        except ValueError as error:
            assert "secret" in str(error)
        else:
            raise AssertionError("secret-bearing memory was accepted")
        assert embedding_backend.calls == backend_calls
        paraphrase = (
            "This step down converter changes supply voltage into regulated output rail")
        try:
            manager.add(paraphrase, tier="ltm", scope="conversation",
                        title="Voltage supply")
        except ValueError as error:
            assert source["id"] in str(error)
            assert "semantic similarity" in str(error)
        else:
            raise AssertionError("semantically duplicate memory was persisted")
        assert len(manager.list(tier="ltm", scope="conversation")) == 1

        unrelated = manager.add(
            "Ground return connects input filter capacitor to power stage reference",
            tier="ltm", scope="conversation", title="Ground return")
        assert unrelated is not None
        manager.add("This step down converter changes supply voltage into regulated output rail",
                    tier="ltm", scope="project", title="Project-specific note")
        try:
            manager.update(
                unrelated["id"],
                "This step down converter changes supply voltage into regulated output rail")
        except ValueError as error:
            assert source["id"] in str(error)
            assert "semantic similarity" in str(error)
        else:
            raise AssertionError("semantic duplicate update was accepted")
        conversation_records = manager.store.list(
            tier="ltm", namespace="semantic-thread", scope="conversation")
        visible_conversation_records = manager.list(tier="ltm", scope="conversation")
        assert conversation_records is not None and conversation_records[0]["id"] == source["id"]
        assert visible_conversation_records is not None
        assert visible_conversation_records[-1]["id"] == unrelated["id"]

    with tempfile.TemporaryDirectory() as directory:
        lexical_only = MemoryManager(
            MemoryStore(Path(directory) / "semantic-disabled.json"),
            thread_id="semantic-thread")
        lexical_only.configure({"ltm": True})
        source = lexical_only.add(
            "A switching regulator converts the input rail into stable output voltage",
            tier="ltm", scope="conversation")
        assert source is not None
        accepted = lexical_only.add(
            "This step down converter changes supply voltage into regulated output rail",
            tier="ltm", scope="conversation")
        assert accepted is not None
        assert accepted["id"] != source["id"]

    with tempfile.TemporaryDirectory() as directory:
        unavailable = MemoryManager(
            MemoryStore(Path(directory) / "semantic-unavailable.json"),
            thread_id="semantic-thread")
        unavailable.set_embedding_backend(FailingEmbeddingBackend())
        unavailable.configure({"ltm": True, "semantic": {"enabled": True}})
        first = unavailable.add("A switching regulator converts input power into output voltage",
                                tier="ltm")
        assert first is not None
        try:
            unavailable.add("Store this only after semantic duplicate validation",
                            tier="ltm")
        except ValueError as error:
            assert str(error) == "semantic_duplicate_check_unavailable"
        else:
            raise AssertionError("semantic-enabled write bypassed failed duplicate validation")
        unavailable_records = unavailable.list(tier="ltm")
        assert unavailable_records is not None
        assert [entry["id"] for entry in unavailable_records] == [first["id"]]
        assert unavailable.semantic_state()["status"] == "embedding_failed"

    with tempfile.TemporaryDirectory() as directory:
        unavailable = MemoryManager(
            MemoryStore(Path(directory) / "semantic-not-ready.json"),
            thread_id="semantic-thread")
        unavailable.configure({"ltm": True, "semantic": {
            "enabled": True, "base_url": "http://127.0.0.1:1"}})
        first = unavailable.add("A switching regulator converts input power to output voltage",
                                tier="ltm")
        assert first is not None
        try:
            unavailable.add("A second record needs duplicate validation", tier="ltm")
        except ValueError as error:
            assert str(error) == "semantic_duplicate_check_unavailable"
        else:
            raise AssertionError("unready semantic backend allowed an unchecked write")
        unavailable_records = unavailable.list(tier="ltm")
        assert unavailable_records is not None
        assert [entry["id"] for entry in unavailable_records] == [first["id"]]
        assert unavailable.semantic_state()["ready"] is False

    with tempfile.TemporaryDirectory() as directory:
        fallback = MemoryManager(MemoryStore(Path(directory) / "unexpected-error.json"))
        fallback.set_embedding_backend(FailingEmbeddingBackend())
        fallback.configure({"ltm": True, "semantic": {"enabled": True}})
        lexical_record = fallback.add("voltage converter powers input rail", tier="ltm")
        assert lexical_record is not None
        result = fallback.retrieve("voltage converter", limit=2)
        assert result is not None
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
