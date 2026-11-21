# Settings and crash-handling contract index

Status: compatibility index (superseded by split domain contracts)

The former combined contract has moved into two normative domain contracts:

- [Settings, theming, live reload, and close worker settlement](Core_Settings.md) — `%LOCALAPPDATA%\RedTilde` JSON5 settings files (`RedTilde.settings.json` / `RedTilde-debug.settings.json`), [SettingsStore.schema.json](../../SettingsStore.schema.json), cold migration and `.invalid-*` backup, field fallback, coalesced `SettingsWatcher` live reload, startup precedence, and Close/`WM_ENDSESSION` worker settlement.
- [Local crash handling and process failure boundaries](Core_CrashHandling.md) — local minidumps, `pending.txt`, retention, DxUi API-5 protocol-2 failure reporting, process-owner UIA/COM teardown drain, checked capacity/codec boundaries, and isolated crash/sanitizer probes.
