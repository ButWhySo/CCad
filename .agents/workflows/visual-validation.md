---
description: This workflow should run everytime.It is a working guide
---

### Phase 0: Ingest agent-methodology, readme, agents.md , progess.md and other markdowns and docs fully first and other things that indicate progressand see the state of repo and what you need to work on. DirTree or similar for ease.
### Phase 1: Preparation & Branching
1. **Determine Scope:** Determine a specific, small set of things you will work upon for the commits in this branch.
2. **Branch Creation:** Branch to the appropriate feature branch. If it does not exist, create it from `main` using standard git/github norms (e.g., `sprint-<number>-<topic>`).
3. **Map Dependencies:** Before writing any code, map all incoming and outgoing dependencies of the target functions/files to avoid breaking existing callers.
4. **Sync Progress:** Read `docs/devops/progress.md` to establish the current phase/sprint context.
5. **Keep backlokgs updated:** Always keep backlogs updated and treat every feature or todo as a backlog. any thing you plan should directly go into backlog. Keep all places where you store backlog consistent, especially backlog.md . Sometimes backlog.md maybe out of shape and not constistent, it is your duty to update it. Check if there is anything in repo that mentiones failed ci/ct/cd. take that into account and fix it alongside current task.
verfiy gui runs properly and doesn't crash from previous coomit/progress by running $env:PATH = "C:\Qt\6.11.1\mingw_64\bin;$env:PATH"
.\build-qt\ccad_gui.exe and waiting a few seconds and interacting with 2-3 elements in gui.

### Phase 2: Research & Implementation
5. **Research First:** Look up the internet for existing works, projects, and standard implementations. Adapt them strictly to our specific LLM-native CAD use-case rather than reinventing the wheel.You have a local copy of kicad repo from gitlab in F:\kicad_src or something , refer to that and learn from their implementations and adapt their implementations to work as per our requirement.
6. **Test-First Development:** Write or update the C++ behavior tests in the `tests/` directory to define what the new code should do before (or alongside) implementing the behavior.
7. **Write Decentralized Code:** Implement the changes. Ensure the GUI remains thin,yet solid(atleast on par or same as kica) and core logic stays inside the `ccad_core` kernel. Adhere to single-responsibility and split files when they become too large.Remember that this is supposed to be an agent first CAD design software..

### Phase 3: Verification Gate (The "Proof")
8. **Moderated Automated Testing:** Test features before committing (both CLI and GUI). Since C++ builds take time, plan your tests in moderation (e.g., test every few commits). However, you MUST run the full CMake build and CTest gate before merging to `main` and pushing to remote.
9. **GUI Visual Validation:** If visual/GUI components are touched, run the official test harness (`scripts/run_sprint_demo.ps1`) to load the board, place the component, wait the current single-preview settle time of 7 seconds, and capture the screenshot. For multi-target GUI validation, use the app-owned target harness with a 5-second initial load wait and fast per-action waits around 800 ms, unless a specific feature needs a longer explicit wait. Intercept `stdout`/`stderr` to catch underlying Qt crashes.
10. **Ingest Verification:** Use image parsing tools (`view_file` on the resulting `.png`) to visually guarantee the UI layout and rendering behave exactly as expected.Use our custom scripts to test the interaction with gui and the gui map proper generation and then using the gui map to interact with the component you just worked on if this applies, because many times small gui tests may pass, but the final build craches on itself when run for more then 20 seconds

### Phase 4: Documentation & Cleanup
11. **Update Amnesia Docs:** Update `docs/codebase-map.md`, `docs/features/implemented-features.md`, and any handover notes immediately in the same sprint. Do not delay documentation.
12. **Deliverable Hygiene:** Purge all intermediate `.tmp` logs or WIP screenshots. Keep only the final verified artifacts.

### Phase 5: Commit & Merge
13. **Structured Commit Message:** Stage changes and write a detailed multi-line commit message including: `Why`, `Changed`, `Behavior`, `Verification`, and `Demo`.
14. **Status Report:** Provide the user with the required status update format: `Progress: Phase X/Y, Sprint N, <branch>, <status>`.
15. **Merge:** Once the branch is completely green, visually proven, and fully documented, merge back to `main`. 
16. **After successfull merge:** Cleanup the branch once its purpose is served and clean up an other stray branches whose purpose is served and may not be needed. 

Keep user posted on your progress and activity, not too verbose, moderately.