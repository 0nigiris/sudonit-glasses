# Contributing to Sudonit Smart Glasses

Thanks for your interest. Sudonit is an open, modular smart-glasses platform.
This guide is intentionally short — it will grow as the project does.

## Before you start

Read the project's source of truth, in this order:

1. `VISION.md` — why Sudonit exists, principles, the highest-level authority
2. `PRODUCT.md`, `ARCHITECTURE.md`, `ROADMAP.md`
3. `PROTOCOL.md` + `protocol/TRANSPORT.md` — the communication contract
4. `DECISIONS.md` — why things are the way they are
5. `DEVELOPMENT.md` — how to build, run, and test the prototype

If a change contradicts `VISION.md`, `VISION.md` wins — or the change needs to
update `VISION.md` first, deliberately.

## Principles (from CLAUDE.md / VISION.md)

- Prefer maintainability, readability, modularity, and documentation over clever
  code. If a simple solution works, use the simple solution.
- Don't add complexity unless it solves a real user problem.
- Treat all user data as sensitive. Never hardcode secrets.
- Priority order: reliability > battery > comfort > performance > features.

## Workflow

1. **Discuss first for anything non-trivial** — open an issue describing the
   problem and proposed approach before writing a large change.
2. **Branch** off `main`; keep changes focused.
3. **Match the surrounding code** — naming, structure, comment density.
4. **Document as you go.** If you add a protocol message, register it in
   `protocol/messages.py` (`KNOWN_TYPES`) **and** document it in `PROTOCOL.md`
   before implementing it — this rule is non-negotiable.
5. **Update the docs you touch** — `ROADMAP.md` (includes completed milestones),
   `DECISIONS.md` (for notable technical decisions).

## Tests

Run the suite before opening a pull request:

```bash
python3 -m pytest -q
```

Add tests for new protocol message types and pipeline behaviour. Tests must pass
with **no API key and no network** — use the deterministic stub provider, not a
live model call, for anything in `tests/`.

## Pull requests

- Describe **what** changed and **why**, and reference the issue.
- Note any decision worth preserving so it can be added to `DECISIONS.md`.
- Keep unrelated changes out of the PR.

## Where help is especially valuable

Embedded (ESP32-S3) firmware, Android development, UX, hardware engineering, and
documentation. See `README.md` for the broader picture.

## License

By contributing, you agree that your contributions are licensed under the
Apache License 2.0 (`LICENSE`).
