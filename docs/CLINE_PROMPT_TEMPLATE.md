# Cline Context Contract Template

Use this template verbatim when starting a task.

## Allowed Files
- docs/AI_CONTEXT.md
- docs/AI_STATE.md
- docs/AI_FILEMAP.md
- (plus any files explicitly provided by the user)

## Rules (verbatim)
- “Do NOT scan repository.”
- “Read only ALLOWED FILES.”
- “Do NOT re-open already read files unless user says they changed.”
- “If you need more files, STOP and ask with 1–3 exact paths + reason.”
- “Output unified diff only; no refactors; no formatting-only changes.”

## Task Intake
- Task goal:
- Constraints:
- Files to read (allowed):

## Execution Checklist
- Confirm allowed files are sufficient.
- If not, request exact file paths (1–3 max) with reason.
- Provide output as unified diff only.

## Definition of Done
- Builds without adding new dependencies.
- No per-tick added unless explicitly requested.
- Timers/Hz values configurable.
- Logs only behind debug flags.
- Only touched allowed files.
- Output diffs only.