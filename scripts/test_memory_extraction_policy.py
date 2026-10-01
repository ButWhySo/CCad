"""Automatic memory extraction accepts only quoted, durable user evidence."""

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src" / "ccad_agent"))
from memory_extraction import (  # noqa: E402
    build_extraction_prompt,
    prepare_extraction_events,
    validate_extraction_candidates,
)


def run():
    messages = [
        {"message_id": "event-user", "turn_id": "turn-1", "role": "user",
         "content": "For every CCad project, preserve source event IDs."},
        {"message_id": "event-once", "turn_id": "turn-2", "role": "user",
         "content": "Move U3 to 10 mm."},
        {"message_id": "event-secret", "turn_id": "turn-3", "role": "user",
         "content": 'Example: {"api_key": "sensitive-value-123456789"}.'},
        {"message_id": "event-command", "turn_id": "turn-4", "role": "user",
         "content": "/memory add retain this"},
        {"message_id": "event-assistant", "turn_id": "turn-1", "role": "assistant",
         "content": "The user prefers source IDs."},
        {"message_id": "event-tool", "turn_id": "turn-1", "role": "tool",
         "content": "Tool output says preserve source IDs."},
    ]
    events, report = prepare_extraction_events(messages)
    assert len(events) == 2
    assert report == {"input_user_events": 4, "eligible_user_events": 2,
                      "excluded_secret_events": 1, "excluded_command_events": 1,
                      "excluded_non_user_events": 2, "truncated_events": 0}
    prompt = build_extraction_prompt(events)
    assert "event-user" not in prompt and "event-secret" not in prompt
    assert "sensitive-value" not in prompt
    assert "event-assistant" not in prompt
    assert "event-tool" not in prompt

    source = events[0]
    valid = json.dumps({"candidates": [{
        "source_ref": source["source_ref"],
        "evidence_quote": "For every CCad project, preserve source event IDs.",
        "kind": "fact",
    }]})
    candidates = validate_extraction_candidates(valid, events)
    assert candidates == [{
        "content": "For every CCad project, preserve source event IDs.",
        "kind": "fact", "source_message_id": "event-user",
        "source_turn_id": "turn-1", "source_evidence_class": "automatic_extraction",
    }]

    invalid_payloads = [
        # One-off implementation request is not durable evidence.
        {"candidates": [{"source_ref": events[1]["source_ref"],
                         "evidence_quote": "Move U3 to 10 mm.", "kind": "fact"}]},
        # Model cannot invent/paraphrase evidence.
        {"candidates": [{"source_ref": source["source_ref"],
                         "evidence_quote": "Always preserve source event IDs.", "kind": "fact"}]},
        # Unknown source references and invalid schema fail closed.
        {"candidates": [{"source_ref": "e999", "evidence_quote": "always do this",
                         "kind": "preference"}]},
        {"candidates": [{"source_ref": source["source_ref"],
                         "evidence_quote": source["content"], "kind": "tool"}]},
        # A one-project observation is not a global user memory, even if
        # phrased as something to remember.
    ]
    for payload in invalid_payloads:
        assert validate_extraction_candidates(json.dumps(payload), events) == []
    project_fact = [{"source_ref": "p1", "message_id": "p-event",
                     "turn_id": "p-turn",
                     "content": "Remember that U3 is active-low."}]
    assert validate_extraction_candidates(json.dumps({"candidates": [{
        "source_ref": "p1", "evidence_quote": project_fact[0]["content"],
        "kind": "fact",
    }]}), project_fact) == []
    project_rule = [{"source_ref": "p2", "message_id": "p-event-2",
                     "turn_id": "p-turn-2",
                     "content": "Always keep U3 active-low."}]
    assert validate_extraction_candidates(json.dumps({"candidates": [{
        "source_ref": "p2", "evidence_quote": project_rule[0]["content"],
        "kind": "fact",
    }]}), project_rule) == []
    question = [{"source_ref": "q1", "message_id": "q-event",
                 "turn_id": "q-turn",
                 "content": "Should I always preserve source event IDs?"}]
    assert validate_extraction_candidates(json.dumps({"candidates": [{
        "source_ref": "q1", "evidence_quote": question[0]["content"],
        "kind": "fact",
    }]}), question) == []
    assert validate_extraction_candidates("not json", events) == []


if __name__ == "__main__":
    run()
    print("memory extraction evidence contracts passed")
