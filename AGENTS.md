# StreamView Agent Rules

These rules apply to AI-assisted development in this repository.

## Scope Discipline

- Do not attempt to implement a full StreamEye replacement in one change.
- Do not implement complete H.264/H.265 parsing at once. Add syntax support in
  small, testable slices.
- Do not introduce new third-party dependencies without documenting the reason in
  `docs/ADR/`.
- Keep parser code independent from GUI and FFmpeg adapters.

## Parser Correctness

- Every parser feature must have tests or golden output.
- Do not use "FFmpeg can decode it" as proof that our parser is correct.
- Parsing code must be bounds-checked and must not read past buffer ends.
- Avoid infinite scans on malformed input.
- When adding codec syntax fields, document the standard source in code comments
  or `docs/CODEC-SCOPE.md`.

## Change Hygiene

- Touch only files needed for the current task.
- Do not reformat unrelated code.
- Do not delete existing code or comments unless the task requires it.
- Prefer small modules with explicit responsibilities.

## Verification

- Run relevant tests before claiming completion.
- Parser changes must include malformed-input coverage where practical.
- CLI output changes must update golden tests.
