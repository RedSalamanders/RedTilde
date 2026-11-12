# Native compiler metadata isolation

Implementation71ecdfb107986059c96af52c1d902970e4de2fe2 runs native clean/build
and pinned desktop-lease compilation through engine5233b527's private compiler
metadata namespace. Original build/source/SDK checks and foreground ownership remain.

[Qualification](qualification.json) records complete x64 Debug, Release and ASan
native profiles, the actual ASan detector, all nine canonical keyboard/focus checks
on each profile and all three ARM64 cross-builds. Each foreground lease verified
restoration of foreground, focus and cursor. Builds passed with zero warnings/errors.
All61 scope/public-build controls, evaluated pinned-lease policy, complete validation,
compiler-free checks and canonical formatting also passed.

An initial inert public-build fixture lacked the new shared launcher boundary;
its preserved failure led to an explicit recording adapter, with assertions intact.
Cross-builds do not qualify ARM64 runtime, and concurrent work supplies no controlled
performance claim. Hosted qualification and the organization plan remain separate.
