# Agent Methodology

This file is general guidance for LLM agents working in any serious codebase. It is not only about CCad.

Update this file when the user gives a reusable rule about how agents should work.

## Keep Learning From The User

When the user teaches a method, preference, or working rule that can apply outside one task, write it down here.

Examples:

- How to plan work.
- How to use git.
- How to test.
- How to document progress.
- How to use screenshots.
- How to work with other agents.
- How to keep a codebase easy for future agents to understand.

Do not record private secrets here. Do not record one-time task details that only matter for a single sprint.

## Write For Future Agents With No Memory

Assume the next agent has no memory of the chat.

Write simple sentences. Avoid buzzwords and unexplained jargon. It is better to use a few more words than to be vague.

Every important project should have:

- A current progress file.
- A codebase map.
- A handover file.
- Clear run and test commands.
- A record of what was built and why.

## Work In Small Verified Sprints

Do not make one huge unsafe change.

Use a branch for a sprint. Make the goal clear. Write tests first when behavior changes. Run focused tests while building. Run the full test gate before commit and before merge.

At every commit and merge, report:

- Phase and sprint number.
- Branch name.
- What changed.
- What tests ran.
- Whether a screenshot or demo artifact exists.

## Use Other Agents When Work Can Run In Parallel

Use subagents when there are independent tasks that can be done without blocking each other.

Good subagent tasks:

- Inspect one subsystem.
- Compare UI references.
- Review architecture.
- Check a test area.
- Propose a bounded refactor.

Do not give multiple agents the same write area unless the files are clearly separated.

Close agents when their work is done.

## Visual Work Needs Visual Proof

For GUI work, do not rely only on code review.

Build the app. Launch it. Capture screenshots. Look at the screenshots. If the screenshot shows a bad layout, fix it before committing.

Screenshot outputs should be ignored by git unless the project intentionally stores golden images.

## Prefer Real Product Shape Early

When building a serious app, the first UI should point toward the real product shape.

For a CAD tool, that means a large canvas, docked panels, toolbars, status readouts, zoom, pan, and object/layer panels. A tiny preview inside a dashboard is not enough if the target is an editor.

## Keep Code Split By Purpose

Do not keep adding code to one large file just because it is fast.

Split code when a part has a clear job:

- Window shell.
- Canvas view.
- Renderer.
- Project summary panel.
- Diagnostics panel.
- Inspector panel.
- Data model.
- Command parser.

Each file should be easy to explain in one or two sentences.

## Keep Language Plain

Use direct language in docs.

Avoid making the next agent decode clever phrases. Say what the rule is, why it matters, and how to check it.
