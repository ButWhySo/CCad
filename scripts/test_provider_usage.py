"""Contracts for safe normalization of provider-reported token usage."""

from pathlib import Path
import sys
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ccad_agent"))

from provider_usage import (account_response, collect_turn_usage,
                            langfuse_usage_details, normalize_usage,
                            with_provider_usage)


def test_canonical_langchain_usage():
    assert normalize_usage({"input_tokens": 41, "output_tokens": 7,
                            "total_tokens": 48, "private": "not copied"}) == {
        "input_tokens": 41, "output_tokens": 7, "total_tokens": 48}


def test_provider_response_usage_aliases():
    assert normalize_usage({"prompt_tokens": 13, "completion_tokens": 5}) == {
        "input_tokens": 13, "output_tokens": 5, "total_tokens": 18}
    assert normalize_usage({"promptTokenCount": 19,
                            "candidatesTokenCount": 3,
                            "totalTokenCount": 22}) == {
        "input_tokens": 19, "output_tokens": 3, "total_tokens": 22}


def test_rejects_invalid_and_non_integer_counts():
    for value in (True, -1, 1.5, "12", 1_000_000_001):
        assert normalize_usage({"input_tokens": value}) == {}
    assert normalize_usage({"private": "api_key=must-not-propagate"}) == {}


def test_response_metadata_shapes_are_read_without_copying_other_fields():
    response = SimpleNamespace(response_metadata={
        "token_usage": {"prompt_tokens": 23, "completion_tokens": 11,
                        "authorization": "must-not-propagate"},
        "request_id": "private-provider-id",
    })
    assert normalize_usage(response) == {
        "input_tokens": 23, "output_tokens": 11, "total_tokens": 34}


def test_turn_usage_excludes_previous_turns_and_aggregates_current_generations():
    class Message:
        def __init__(self, kind, usage=None):
            self.type = kind
            self.usage_metadata = usage or {}

    messages = [
        Message("human"), Message("ai", {"input_tokens": 900,
                                           "output_tokens": 20}),
        Message("human"), Message("ai", {"input_tokens": 31,
                                           "output_tokens": 8}),
        Message("ai", {"prompt_tokens": 5, "completion_tokens": 2}),
    ]
    assert collect_turn_usage(messages) == {
        "input_tokens": 36, "output_tokens": 10, "total_tokens": 46}


def test_turn_without_authoritative_usage_is_unavailable():
    class Message:
        def __init__(self, kind):
            self.type = kind
            self.content = "do not inspect or echo"

    assert collect_turn_usage([Message("human"), Message("ai")]) == {}
    assert collect_turn_usage([Message("ai")]) == {}


def test_usage_report_keeps_estimate_and_adds_only_safe_provider_counts():
    report = with_provider_usage({
        "provider": "openrouter", "model": "model-id",
        "estimated_input_tokens": 48, "content_emitted": False,
        "secret_value_visible": False,
    }, {"input_tokens": 51, "output_tokens": 9, "total_tokens": 60})
    assert report["estimated_input_tokens"] == 48
    assert report["provider_usage"] == {
        "input_tokens": 51, "output_tokens": 9, "total_tokens": 60}
    assert report["input_token_estimate_delta"] == 3
    assert report["provider_usage_source"] == "provider_response"
    assert report["content_emitted"] is False
    assert "private" not in str(report)


def test_accounting_reads_only_provider_usage_from_response():
    response = SimpleNamespace(
        usage_metadata={"input_tokens": 12, "output_tokens": 4,
                        "total_tokens": 16},
        content="sensitive model response text",
        response_metadata={"request_id": "must-not-propagate"})
    report = account_response({"estimated_input_tokens": 15}, response)
    assert report["provider_usage"] == {
        "input_tokens": 12, "output_tokens": 4, "total_tokens": 16}
    assert "sensitive model response text" not in str(report)
    assert "must-not-propagate" not in str(report)


def test_langfuse_usage_details_use_sdk_field_names_and_safe_integers():
    assert langfuse_usage_details({
        "input_tokens": 12, "output_tokens": 4, "total_tokens": 16,
        "authorization": "must-not-propagate",
    }) == {"input": 12, "output": 4, "total": 16}


if __name__ == "__main__":
    for test in (test_canonical_langchain_usage,
                 test_provider_response_usage_aliases,
                 test_rejects_invalid_and_non_integer_counts,
                 test_response_metadata_shapes_are_read_without_copying_other_fields,
                 test_turn_usage_excludes_previous_turns_and_aggregates_current_generations,
                 test_turn_without_authoritative_usage_is_unavailable,
                 test_usage_report_keeps_estimate_and_adds_only_safe_provider_counts,
                 test_accounting_reads_only_provider_usage_from_response,
                 test_langfuse_usage_details_use_sdk_field_names_and_safe_integers):
        test()
    print("PASS provider token-usage contracts; no network")
