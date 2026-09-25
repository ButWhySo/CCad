"""Bounded, opt-in embeddings through a loopback Ollama service."""

from __future__ import annotations

import json
import math
import re
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import ProxyHandler, Request, build_opener


class EmbeddingError(RuntimeError):
    """Safe embedding failure category; never includes request text or URL."""

    def __init__(self, category: str):
        self.category = category
        super().__init__(category)


class OllamaEmbeddingBackend:
    MAX_TEXTS = 64
    MAX_TEXT_CHARS = 16_384
    MAX_DIMENSIONS = 16_384
    _model_name = re.compile(r"[A-Za-z0-9][A-Za-z0-9._:/-]{0,127}\Z")

    def __init__(self, base_url="http://127.0.0.1:11434", model="embeddinggemma"):
        self.base_url = self._validate_base_url(base_url)
        self.model = self._validate_model(model)
        self.model_version = ""
        self._http = build_opener(ProxyHandler({}))

    @staticmethod
    def _validate_base_url(value: str) -> str:
        parsed = urlsplit(str(value).strip())
        if (parsed.scheme != "http" or parsed.hostname not in {"127.0.0.1", "::1"}
                or parsed.username or parsed.password or parsed.query or parsed.fragment
                or parsed.path not in ("", "/")):
            raise EmbeddingError("embedding_endpoint_not_local")
        try:
            port = parsed.port
        except ValueError as error:
            raise EmbeddingError("embedding_endpoint_invalid") from error
        if port is not None and not 1 <= port <= 65535:
            raise EmbeddingError("embedding_endpoint_invalid")
        host = "[::1]" if parsed.hostname == "::1" else parsed.hostname
        return f"http://{host}{':' + str(port) if port else ''}"

    @classmethod
    def _validate_model(cls, value: str) -> str:
        model = str(value).strip()
        if not cls._model_name.fullmatch(model):
            raise EmbeddingError("embedding_model_invalid")
        return model

    @property
    def identity(self) -> str:
        return f"ollama:{self.base_url}:{self.model}:{self.model_version}"

    def _request(self, path: str, payload: dict | None = None, *, timeout=2):
        data = None if payload is None else json.dumps(payload).encode("utf-8")
        request = Request(self.base_url + path, data=data,
                          headers={"Content-Type": "application/json"} if data else {},
                          method="GET" if data is None else "POST")
        try:
            with self._http.open(request, timeout=timeout) as response:
                result = json.loads(response.read(4 * 1024 * 1024))
        except HTTPError as error:
            raise EmbeddingError("embedding_http_error") from error
        except (URLError, TimeoutError, OSError) as error:
            raise EmbeddingError("service_unavailable") from error
        except (json.JSONDecodeError, UnicodeDecodeError) as error:
            raise EmbeddingError("embedding_invalid_response") from error
        if not isinstance(result, dict):
            raise EmbeddingError("embedding_invalid_response")
        return result

    def check_ready(self) -> dict:
        try:
            payload = self._request("/api/tags")
            models = payload.get("models", [])
            if not isinstance(models, list):
                raise EmbeddingError("embedding_invalid_response")
            model = next((item for item in models if isinstance(item, dict) and
                          (item.get("name") == self.model or item.get("model") == self.model)), None)
            if model is None and ":" not in self.model:
                model = next((item for item in models if isinstance(item, dict) and
                              str(item.get("name", item.get("model", ""))).startswith(
                                  self.model + ":")), None)
            if model is None:
                self.model_version = ""
                return {"ready": False, "status": "model_not_installed",
                        "error": "model_not_installed"}
            digest = str(model.get("digest", ""))
            self.model_version = digest[:128]
            if not self.model_version:
                raise EmbeddingError("embedding_model_version_missing")
            return {"ready": True, "status": "ready", "error": "",
                    "model_version": self.model_version}
        except EmbeddingError as error:
            self.model_version = ""
            return {"ready": False, "status": error.category, "error": error.category}

    @classmethod
    def _normalize_vector(cls, value) -> list[float]:
        if not isinstance(value, list) or not 1 <= len(value) <= cls.MAX_DIMENSIONS:
            raise EmbeddingError("embedding_vector_invalid")
        if any(isinstance(item, bool) or not isinstance(item, (int, float))
               for item in value):
            raise EmbeddingError("embedding_vector_invalid")
        try:
            vector = [float(item) for item in value]
        except (TypeError, ValueError, OverflowError) as error:
            raise EmbeddingError("embedding_vector_invalid") from error
        if not all(math.isfinite(item) for item in vector):
            raise EmbeddingError("embedding_vector_invalid")
        norm = math.sqrt(sum(item * item for item in vector))
        if not math.isfinite(norm) or norm <= 1e-12:
            raise EmbeddingError("embedding_vector_invalid")
        return [item / norm for item in vector]

    def embed_documents(self, texts) -> list[list[float]]:
        return self._embed(texts)

    def embed_query(self, text: str) -> list[float]:
        vectors = self._embed([text])
        return vectors[0]

    def _embed(self, texts) -> list[list[float]]:
        texts = list(texts)
        if not texts or len(texts) > self.MAX_TEXTS or any(
                not isinstance(text, str) or not text.strip() or
                len(text) > self.MAX_TEXT_CHARS for text in texts):
            raise EmbeddingError("embedding_input_invalid")
        response = self._request("/api/embed", {
            "model": self.model, "input": texts, "truncate": False,
        }, timeout=30)
        response_model = str(response.get("model", ""))
        if not response_model or response_model != self.model:
            raise EmbeddingError("embedding_model_mismatch")
        rows = response.get("embeddings")
        if not isinstance(rows, list) or len(rows) != len(texts):
            raise EmbeddingError("embedding_invalid_response")
        vectors = [self._normalize_vector(row) for row in rows]
        if len({len(vector) for vector in vectors}) != 1:
            raise EmbeddingError("embedding_dimension_mismatch")
        return vectors
