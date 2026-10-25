# Final app keyboard qualification — 25 October

The [reviewed keyboard packet](summary.json) records all nine x64 Release stages passing at app `da8aafb`
and SDK pin `baff43d8`, with 168 real injected keyboard events. The checks cover editor text/undo, Files,
Find, Settings, Focus, selection tools, exact equation source, File menu and asynchronous close. The clipboard
is untouched. The canonical lease completes with foreground, keyboard focus and pointer unchanged.

Following the person's instruction to start the agreed checks, the harness uses its visible 30-second Start
countdown with Cancel, the existing warning and restoration checks. No confirmation click is injected.
This is an automatic-countdown run, distinct from a normal human-confirmed Start.

Independent replay verifies all nine completed payload files, the exact source identity, SDK pin, executable,
build receipt and lease result. The lead repeats the payload byte/hash replay. The Release executable is
byte-identical to the `fa492ab` functional matrix and `138a648` measurement build; the six `da8aafb` documentation
rebuilds are build-only checks. Publication does not retag those earlier executions.

The first attempt launches the child but fails its composite setup-focus predicate before sending a key.
Its diagnostic does not isolate the failing component; it does not prove an OS foreground refusal. A second
normal prompt expires before child launch. Both attempts are retained separately. The later exact-build pass
does not establish the first failure's cause or claim a source fix.

This focused run does not close physical IME/touch/assistive-technology, performance acceptance, hosted ARM64
or whole-product gates. Private receipts retain desktop details; this packet publishes only outcome and digest
metadata.
