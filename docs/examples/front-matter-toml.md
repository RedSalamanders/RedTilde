+++
title = "Lantern field notes"
author = "Mira Vale"
draft = true
tags = ["observations", "notebook"]

[session]
name = "Evening"
samples = 12
+++

# TOML front matter

Three plus signs fence TOML metadata at the beginning of the document. DxUi's
private TOML parser validates syntax and preserves the exact source. These
values do not configure RedTilde or execute anything.

| Field | Meaning |
| --- | --- |
| `title` | The document's invented name |
| `draft` | Example metadata, independent of the editor's dirty state |
| `session.samples` | An invented count |

Compare the separate [YAML example](front-matter-yaml.md).
