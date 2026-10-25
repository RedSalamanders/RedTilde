# RedTilde keyboard and focus checks at SDK1714

The [run summary](summary.json) records an authorized 30-second automatic Start countdown followed by all nine
keyboard/focus stages and 168 injected events. The x64 Release child exits successfully; foreground and focus
remain unchanged, and the cursor is restored. Independent and lead replay verify all nine retained payload
streams, native outcomes, source identity, exact executable and lease/library bindings.

The executable is byte-identical to the frozen `c60e9ca` binary at exact SDK pin `1714f8b0`. The
[source inventory](source-input-inventory.json) records the distinct dirty-documentation snapshot and its
168 hashed inputs; the three changed documentation files are separately preserved privately. The standalone
build-receipt digest was verified before the next build, but its original bytes were not separately frozen.
The completed run receipt and frozen executable remain retained. This limitation is explicit in the summary.

This is one x64 Release interactive suite. It does not retag the earlier [334/c60 full-suite matrix](../local-qualification-33481d3-c60e9ca/README.md)
or execute later harness/SDK changes. Native ARM64, physical IME/assistive technology, current CI and
[performance acceptance](../performance-c60e9ca/README.md) remain separate gates. No desktop screenshots or
personal fixture data are published.
