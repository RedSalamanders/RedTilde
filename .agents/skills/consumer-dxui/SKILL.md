---
name: consumer-dxui
description: Adopt or update the exact DxUi rich-text pin and its public native APIs in RedTilde.
---

Read [the build contract](../../../Specs/Build/Build_Toolchain.md), `Dependencies/DxUi.lock.json` and the pinned
source's `docs/rich-text.md`/`capabilities.json`. Rich-text PR #89 is merged; API-4 adoption remains experimental
until its separate upstream performance and product qualification gates close.
Use public headers and the imported single-library targets. Never enumerate/copy control sources or include
private headers. Verify the actual pin's source capacity, image callbacks, local-source support and extension
limits before making product claims. A pin update needs fresh consumer editing, storage, capture and resource
evidence; upstream tests cannot substitute. Keep rejected/unsupported content editable as exact source.
