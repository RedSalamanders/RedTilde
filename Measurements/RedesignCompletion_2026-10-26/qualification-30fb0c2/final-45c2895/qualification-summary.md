# RedTilde 45c2895 native checkpoint

This checkpoint binds fresh native evidence to app source `45c2895bd5a298550fe88cb9f905048497be94a7` and SDK `4077f57df8207eac6c39c43437feb45b19d23cf8`. Matrix `sourceIdentity` is recorded separately from compiled-source map `633302c45ac8da29e7bea24cf7909dc70d8aad1665ebe618252dd31f72e0d8f3`; the map conservatively includes `.gitattributes` and two PowerShell files, with no C++ source changes. PE/PDB identities are freshly bound in the evidence metadata and are not reused from 30fb or the restored 10B5 build.

- Six builds passed: x64 Debug, Release and ASan Debug; ARM64 Debug, Release and ASan Debug.
- All three x64 Full runs passed 65 behavioral cases (46 Editor) and produced 23 captures each. The x64 ASan profile detected the intentional heap-use-after-free.
- ARM64 evidence is cross-build only. No ARM64 runtime was executed.
- The separate Release keyboard run passed all nine stages and recorded 168 injected events. Clipboard was untouched; physical IME/AT qualification remains separate. Desktop restoration was verified by the canonical parent lease.
- Hosted RedTilde CI run `40341402876` was blocked before any steps started by the payment/spending restriction. Hosted ARM64 remains open.

Performance acceptance remains open. The three original-UI flags remain Compose command +17.4824%, Split owner cycles +11.1539%, and Split working set +2.0931%. SDK policy acceptance and the cursor lease also remain open. This packet is not a repository-wide or performance-green verdict.

`evidence-metadata.json` contains source, receipt, matrix and artifact hashes. `private-payload-hashes.json` rehashes retained payload files under synthetic labels only; it omits machine paths and raw payload contents. `packet-hashes.json` binds these files and does not hash itself.