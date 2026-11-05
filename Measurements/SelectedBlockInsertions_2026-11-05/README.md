# Selected quote and code insertions

Status: scoped code, tests, normative contracts and local native qualification complete.

Implementation: `4c967858d4482e45d114e0f4455fe1efefbd2b4a`.
DxUi API 4: `f765aeca5e059dbe768841b605b1be6a5f6ea942`, published through
[SDK PR174](https://github.com/RedSalamanders/DxUi/pull/174).

The actual Insert Quote and Code Block commands transform selected source in Compose, Source and Split.
Tests independently specify exact expected source, including mixed line endings, Unicode, partial lines
and embedded backticks. They retain backwards selection and logical focus, verify one-step Undo/Redo and
saved identity, preserve existing empty-selection templates, and exercise read-only/capacity refusal.
Refusal never replaces selected text with a sample.

The unchanged app command at `1f0cf892` fails the exact selected-content assertion. That original run also
records two foreground-equality failures: both observed other processes, and one entry HWND disappeared.
Those observations do not establish why the foreground changed. Hidden tests now observe all foreground
events belonging to their own process, including retired windows, instead of requiring unrelated desktop
windows to remain unchanged. The observer has a negative control; real interactive focus assertions remain.
Original failures, programs, symbols, receipts and logs are preserved, rather than relabelled as passed.

All six builds have zero warnings/errors. Each x64 profile passes all 83 registered cases, 36 hidden native
captures, process-exit/recovery/crash checks and the ordinary performance smoke: 249 case executions and
108 captures. The real ASan detector passes. ARM64 entries are cross-builds only. Native ARM64, physical
input and paired R10 acceptance remain separately owned.

[The qualification projection](qualification.json) binds every build/run, original counterexample and
member-verified app/SDK archive. All 140 actual compiled inputs are retained, archive SHA-256
`e19fe7658fdc59e1e7272900e59c19d6c239201a7063a2ec2ccf0f114cb81c75`.
Compiled identity is `621e089ea0203d103fd38004134df78d5acce1db93aeee821901cbdb56767b12`.
Final canonical validation and formatting pass. Raw machine paths and compiler contexts remain private.

The durable contract is [Markdown editing](../../Specs/Editor/Editor_Markdown.md); remaining work stays in
the [single active plan](../../Specs/Plans/WIP/RemainingWork_2026-11-04.md).
