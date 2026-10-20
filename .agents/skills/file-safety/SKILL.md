---
name: file-safety
description: Implement RedTilde Unicode open/save, dirty prompts, external-conflict handling and recovery.
---

Read [the file-safety contract](../../../Specs/Core/Core_FileSafety.md). Stage/validate before replacing the
current document. Preserve BOM/EOLs, reject NUL/invalid UTF-8 and the actual editor capacity. Save with an owned
same-directory temporary, flush and atomic replace; failed or cancelled operations preserve dirty state/path.
Check external identity and require an explicit overwrite choice. Worker completion must match the session
and saved snapshot; edits after admission remain dirty. Recovery is per-session and never overwrites originals.
Use isolated directories for failure/conflict/cancel tests; never touch the user's unrelated files.
