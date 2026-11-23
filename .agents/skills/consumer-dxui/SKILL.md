---
name: consumer-dxui
description: Adopt or update the exact DxUi rich-text pin and its public native APIs in RedTilde. Use when changing Dependencies/DxUi.lock.json, adopting a new DxUi API or capability, or making a product claim that depends on what the pinned rich-text library supports.
---

Read [the build contract](../../../Specs/Build/Build_Toolchain.md), `Dependencies/DxUi.lock.json` and the pinned
source's `docs/rich-text.md`, `docs/upgrade.md` and `capabilities.json`. API-5 (`0c371a61`) is the qualified `v1.0.0`
pin in `Dependencies/DxUi.lock.json`; any future API-6 migration remains separate post-1.0 work (RedHome D20)
until fresh consumer failure/native qualification and separate performance/product gates close. API-4
measurements keep their original identity and cannot qualify the API-5 host/adapters.
Use public headers and the imported single-library targets. Never enumerate/copy control sources or include
private headers. Verify the actual pin's source capacity, image callbacks, local-source support and extension
limits before making product claims. A pin update needs fresh consumer editing, storage, capture and resource
evidence; upstream tests cannot substitute. Keep rejected/unsupported content editable as exact source.
