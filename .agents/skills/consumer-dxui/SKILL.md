---
name: consumer-dxui
description: Adopt or update the exact DxUi rich-text pin and its public native APIs in RedTilde.
---

Read [the build contract](../../../Specs/Build/Build_Toolchain.md), `Dependencies/DxUi.lock.json` and the pinned
source's `docs/rich-text.md`, `docs/upgrade.md` and `capabilities.json`. The API-5 migration remains experimental
until fresh consumer failure/native qualification and separate performance/product gates close. API-4
measurements keep their original identity and cannot qualify the API-5 host/adapters.
Use public headers and the imported single-library targets. Never enumerate/copy control sources or include
private headers. Verify the actual pin's source capacity, image callbacks, local-source support and extension
limits before making product claims. A pin update needs fresh consumer editing, storage, capture and resource
evidence; upstream tests cannot substitute. Keep rejected/unsupported content editable as exact source.
