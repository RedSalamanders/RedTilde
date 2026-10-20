---
name: markdown-editing
description: Implement RedTilde rich, source, split and local Markdown editing with shared history and selection.
---

Read [the editor contract](../../../Specs/Editor/Editor_Markdown.md). The main RichTextEditor is authoritative;
never SetText during a view switch. Split is a read-only mirror. Apply source ACPs and scalar/CRLF boundaries,
not rendered string indexes. Prefer public semantic commands; check current syntax/selection before applying a
plan. Mode/local-source commands must respect active IME composition. After contextual commands, preserve the
selection and return focus. Test exact undo, unchanged source during switches, stale/pending plans, Unicode,
literal fallback and saved-content dirty-state restoration. Keep unsupported Markdown intact.
