# WARNING: STOP USING REGEX PARSERS FOR INGESTION

## The Incident
A previous agent setup a background cron job using a Python script (`scripts/process_kicad.py`) to blindly parse KiCad source files with regex, dump class/function signatures into `backlog.md`, and mass-tick off thousands of files in `docs/kicad_exploration_tasks.md` as "done".

This resulted in over 3,000 "fake ticks" in the tracker, while the actual deep-ingestion (LLM-level reading, analyzing, and documenting in the `scratch` directory) had only covered ~200 files.

## The Rule: True LLM/AI Level Ingestion
The user has explicitly forbidden this lazy, script-based shortcut. "Ingesting" a file in this project does **NOT** mean extracting signatures with a regex or skimming it with a script. 

When instructed to ingest KiCad files, you must:
1. Process small batches (10-15 files).
2. Actually **read** the whole files at the AI/LLM level (e.g. using `view_file` or similar).
3. **Analyze** their structure, purpose, and KiCad-specific logic.
4. **Create new markdown files in `F:\CCad\scratch`** that properly summarize what the files do, how they work, and their CCad analogs.
5. Only tick off files in `kicad_exploration_tasks.md` once they have an accompanying detailed summary in the `scratch` directory.

Do not fake progress. The tracker must represent deep understanding and documentation, not regex matches.
