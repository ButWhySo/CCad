"""Small deterministic BM25 scorer for bounded, pre-scoped local documents."""

from __future__ import annotations

import math
import re
from collections import Counter
from typing import Any, Iterable


_TOKEN = re.compile(r"[a-z0-9_]{2,}", re.IGNORECASE)
_K1 = 1.5
_B = 0.75


def _terms(value: Any) -> list[str]:
    return [term.casefold() for term in _TOKEN.findall(str(value or ""))][:4096]


def rank_documents(query: str, documents: Iterable[dict], *, text_key="text",
                   min_matches=1) -> list[dict]:
    """Rank already scope-filtered documents; return BM25 and match provenance."""
    query_terms = list(dict.fromkeys(_terms(query)))[:96]
    rows = list(documents)
    if not query_terms or not rows:
        return []
    token_rows = [_terms(row.get(text_key, "")) for row in rows]
    lengths = [len(terms) for terms in token_rows]
    average_length = sum(lengths) / max(1, len(lengths))
    frequencies = [Counter(terms) for terms in token_rows]
    document_frequency = Counter(term for counts in frequencies for term in counts)
    ranked = []
    for index, (row, counts) in enumerate(zip(rows, frequencies)):
        matched = [term for term in query_terms if counts.get(term, 0)]
        if len(matched) < max(1, min(8, int(min_matches))):
            continue
        score = 0.0
        for term in matched:
            df = document_frequency[term]
            idf = math.log1p((len(rows) - df + 0.5) / (df + 0.5))
            tf = counts[term]
            norm = tf + _K1 * (1.0 - _B + _B * lengths[index] / max(1.0, average_length))
            score += idf * tf * (_K1 + 1.0) / norm
        ranked.append({"document": row, "score": score,
                       "matched_terms": matched, "document_length": lengths[index],
                       "ordinal": index})
    ranked.sort(key=lambda item: (-item["score"], item["ordinal"]))
    return ranked


def _document_id(row: dict) -> str:
    document = row.get("document", {})
    if not isinstance(document, dict):
        return ""
    entry = document.get("entry")
    if isinstance(entry, dict) and entry.get("id"):
        return str(entry["id"])
    return str(document.get("id", ""))


def fuse_rankings(rankings: dict[str, list[dict]], *, weights=None,
                  rrf_k=60) -> list[dict]:
    """Fuse deterministic ranked channels without comparing their raw scores."""
    k = max(1, min(1000, int(rrf_k)))
    weights = weights if isinstance(weights, dict) else {}
    fused: dict[str, dict] = {}
    ordinal = 0
    for channel, rows in rankings.items():
        weight = max(0.0, min(10.0, float(weights.get(channel, 1.0))))
        if not weight:
            continue
        for rank, row in enumerate(rows, 1):
            key = _document_id(row)
            if not key:
                continue
            item = fused.get(key)
            if item is None:
                item = {"document": row["document"], "score": 0.0,
                        "channel_ranks": {}, "matched_terms": [], "ordinal": ordinal}
                fused[key] = item
                ordinal += 1
            item["score"] += weight / (k + rank)
            item["channel_ranks"][str(channel)] = rank
            for term in row.get("matched_terms", ()):
                if term not in item["matched_terms"]:
                    item["matched_terms"].append(term)
    result = list(fused.values())
    result.sort(key=lambda item: (-item["score"], item["ordinal"]))
    return result


def diversify_ranked(ranked: Iterable[dict], *, limit, text_key="text",
                     relevance_weight=0.7, similarity_fn=None) -> list[dict]:
    """Greedily rerank a bounded result set for query relevance and novelty."""
    rows = list(ranked)
    limit = max(0, min(32, int(limit)))
    if not rows or not limit:
        return []
    weight = max(0.0, min(1.0, float(relevance_weight)))
    maximum = max(float(row.get("score", 0.0)) for row in rows) or 1.0
    terms = [_terms(row.get("document", {}).get(text_key, ""))
             for row in rows]
    selected: list[dict] = []
    selected_indices: list[int] = []
    remaining = list(range(len(rows)))
    while remaining and len(selected) < limit:
        best_index = None
        best_value = float("-inf")
        best_redundancy = 0.0
        for index in remaining:
            candidate_terms = set(terms[index])
            if similarity_fn:
                redundancy = max((max(0.0, min(1.0, float(similarity_fn(
                    rows[index]["document"], rows[chosen]["document"]))))
                                  for chosen in selected_indices), default=0.0)
            else:
                redundancy = max((len(candidate_terms & set(terms[chosen])) /
                                  max(1, len(candidate_terms | set(terms[chosen])))
                                  for chosen in selected_indices), default=0.0)
            relevance = float(rows[index].get("score", 0.0)) / maximum
            value = weight * relevance - (1.0 - weight) * redundancy
            if value > best_value or (value == best_value and
                                      (best_index is None or index < best_index)):
                best_index, best_value = index, value
                best_redundancy = redundancy
        assert best_index is not None
        item = dict(rows[best_index])
        item["diversity_score"] = round(best_value, 8)
        item["redundancy_score"] = round(best_redundancy, 8)
        selected.append(item)
        selected_indices.append(best_index)
        remaining.remove(best_index)
    return selected
