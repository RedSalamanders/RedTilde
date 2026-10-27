# RedTilde 30fb native qualification


- [Fresh 45c2895 native checkpoint](final-45c2895/README.md) records six fresh builds, three passing x64 Full profiles (65 behavioral cases and 23 captures each), real x64 ASan detection, ARM64 cross-builds only, and a separately leased 9-stage/168-event keyboard run. This follow-up binds its own source, compiled-source-map, PE/PDB and receipt identities. Performance acceptance, hosted ARM64, SDK policy acceptance and cursor lease remain open. See its packet manifest for exact hashes.
This review packet summarizes the completed x64 native matrix, ARM64 cross-builds, and the separately leased keyboard run for the pinned RedTilde/DxUi sources. It keeps performance acceptance open and records the hosted billing block.

- [Publication summary](publication-summary.md)
- [Private retained-evidence hash index](private-evidence-hashes.md)
- [Restored colored Release checkpoint](restored-release.json) records fresh Full and keyboard runs after the bounded color-cost experiment. Its executable/PDB differ from the original matrix; source and compiled inputs are restored exactly and the two artifact sets remain separate.

`packet-hashes.json` binds this README, the publication summary, the private hash index, and the restored Release checkpoint. It does not hash itself. The original matrix Release executable remains `FE192B9E`; the separately rebuilt restored Release executable is `10B5B8BD`, with its own PDB and build/run receipts. Neither artifact set is retagged as the other.

The packet contains metadata, counts, and hashes only. It does not copy raw test payloads, fixture documents, executable/PDB artifacts, or C++ sources. Evidence labels in the private index are synthetic and omit machine paths and original fixture filenames.
