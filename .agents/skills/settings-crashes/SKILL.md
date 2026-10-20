---
name: settings-crashes
description: Implement RedTilde live settings, local crash reports and restart diagnostics without telemetry.
---

Read [the settings/crash contract](../../../Specs/Core/Core_SettingsAndCrashes.md). Settings have field-level
fallback and atomic persistence; errors leave editing available. Apply changes live and honor high contrast.
Initialize crash-path state before UI/workers; use fixed buffers and preloaded DbgHelp at a fault. Keep dump
retention/marker inspection in healthy startup. Never upload automatically or attempt recovery writes in the
fault handler. Crash probes run in isolated child processes/data roots. Verify dump and pending marker, ordinary
restart behavior, invalid settings and real ASan detection separately.
