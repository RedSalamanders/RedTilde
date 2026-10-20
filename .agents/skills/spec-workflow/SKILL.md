---
name: spec-workflow
description: Maintain RedTilde domain contracts, one indexed WIP plan, capability status and completed-plan handoff.
---

Start with [spec authority](../../../Specs/README.md). Put lasting behavior in its domain; plans own sequencing,
decisions, acceptance scenarios and evidence links. Reconcile new code/tests/docs together. Mark implemented,
experimental and pending separately in capabilities.json. Do not promote an upstream preview to product
qualification. Keep unresolved language compatibility, native platform, physical input and release gates active. Move the plan
to Done only when every in-scope implementation/test/doc obligation closes. Run validate.ps1 for links/indexes,
skill metadata, lock and build inventory; record one Changes fragment.
