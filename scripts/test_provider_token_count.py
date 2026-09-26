"""No-network contracts for exact Gemini full-request token preflight."""

from pathlib import Path
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from provider_token_count import count_gemini_input_tokens


class FakeProviderClient:
    def __init__(self, response=None, error=None):
        self.response = response
        self.error = error
        self.calls = []

    def count_tokens(self, *, request, timeout, retry):
        self.calls.append((request, timeout, retry))
        if self.error:
            raise self.error
        return self.response


def binding(provider_client, prepared_request=None, error=None):
    class Model:
        model = "gemini-2.5-flash"
        def _prepare_request(self, messages, **kwargs):
            self.received = (messages, kwargs)
            if error:
                raise error
            return prepared_request, {}

    model = Model()
    model.client = provider_client
    return SimpleNamespace(bound=model, kwargs={"tools": [{"function_declarations": []}]},
                           _test_model=model)


def test_disabled_and_non_gemini_never_call_count_endpoint():
    provider = FakeProviderClient(SimpleNamespace(total_tokens=42))
    llm = binding(provider, {"contents": ["full prompt"]})
    disabled = count_gemini_input_tokens(
        llm, ["message"], "google_gemini", "gemini-2.5-flash", False, 12,
        lambda error: "provider_unavailable")
    unsupported = count_gemini_input_tokens(
        llm, ["message"], "openai", "gpt-5.1", True, 12,
        lambda error: "provider_unavailable")
    assert disabled == {"status": "disabled", "source": "unavailable",
                        "additional_request_sent": False}
    assert unsupported["status"] == "unsupported_provider"
    assert unsupported["additional_request_sent"] is False
    assert provider.calls == []


def test_count_uses_bound_full_messages_tools_model_and_timeout():
    response = SimpleNamespace(total_tokens=1842)
    provider = FakeProviderClient(response)
    messages = ["system prompt with project context", "user request"]
    request = {"contents": ["provider-shaped full prompt"], "tools": ["real schema"]}
    llm = binding(provider, request)
    result = count_gemini_input_tokens(
        llm, messages, "google_gemini", "gemini-2.5-flash", True, 17,
        lambda error: "provider_unavailable")
    assert result == {"status": "counted", "source": "gemini_count_tokens",
                      "exact_input_tokens": 1842, "additional_request_sent": True}
    assert provider.calls == [({"model": "models/gemini-2.5-flash",
                                "generate_content_request": request}, 17, None)]
    prepared_messages, kwargs = llm._test_model.received
    assert prepared_messages is messages
    assert kwargs["tools"] == llm.kwargs["tools"]


def test_google_adapter_single_object_request_shape_is_supported():
    request = SimpleNamespace(model="gemini-2.5-flash",
                              system_instruction="system", contents=["turn"],
                              tools=["function declaration"])
    provider = FakeProviderClient(SimpleNamespace(total_tokens=73))
    llm = binding(provider, request)
    result = count_gemini_input_tokens(
        llm, ["message"], "google_gemini", "gemini-2.5-flash", True, 11,
        lambda error: "provider_unavailable")
    assert result["exact_input_tokens"] == 73
    sent = provider.calls[0][0]["generate_content_request"]
    assert sent is request
    assert sent.system_instruction == "system"


def test_count_failure_is_safe_and_records_whether_request_was_sent():
    provider = FakeProviderClient(error=RuntimeError("authorization=private"))
    llm = binding(provider, {"contents": ["secret-bearing prompt"]})
    result = count_gemini_input_tokens(
        llm, ["private"], "google_gemini", "gemini-2.5-flash", True, 9,
        lambda error: "quota_exhausted")
    assert result == {"status": "failed", "source": "unavailable",
                      "additional_request_sent": True,
                      "error_category": "quota_exhausted"}
    assert "private" not in str(result)


def test_invalid_model_count_response_never_claims_exactness():
    for total in (None, True, -1, 1.5, "42", 1_000_000_001):
        provider = FakeProviderClient(SimpleNamespace(total_tokens=total))
        llm = binding(provider, {"contents": ["full prompt"]})
        result = count_gemini_input_tokens(
            llm, [], "google_gemini", "gemini-2.5-flash", True, 5,
            lambda error: "provider_unavailable")
        assert result["status"] == "failed"
        assert "exact_input_tokens" not in result
        assert result["additional_request_sent"] is True


def test_preparation_failure_does_not_contact_google():
    provider = FakeProviderClient(SimpleNamespace(total_tokens=11))
    llm = binding(provider, error=ValueError("private prompt"))
    result = count_gemini_input_tokens(
        llm, ["message"], "google_gemini", "gemini-2.5-flash", True, 5,
        lambda error: "invalid_request")
    assert result["status"] == "failed"
    assert result["additional_request_sent"] is False
    assert result["error_category"] == "invalid_request"
    assert provider.calls == []


def test_model_identity_must_be_a_provider_model_id():
    provider = FakeProviderClient(SimpleNamespace(total_tokens=1))
    llm = binding(provider, {"contents": ["full prompt"]})
    for model in ("", "models/", "models/a/b", "../model"):
        result = count_gemini_input_tokens(
            llm, [], "google_gemini", model, True, 5,
            lambda error: "provider_unavailable")
        assert result["status"] == "failed"
        assert result["additional_request_sent"] is False
    assert provider.calls == []


if __name__ == "__main__":
    tests = [value for name, value in globals().copy().items()
             if name.startswith("test_") and callable(value)]
    for test in tests:
        test()
    print(f"PASS {len(tests)} Gemini full-request token-count contracts; no network")
