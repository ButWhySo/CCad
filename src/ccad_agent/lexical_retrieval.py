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
