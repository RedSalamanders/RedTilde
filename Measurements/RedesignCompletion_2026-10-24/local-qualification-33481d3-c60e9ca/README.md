# RedTilde local qualification at 33481d3 and c60e9ca

The [sanitized summary](summary.json) binds native source `33481d3372f529fd28827ac98c56095d65fbb61d` and exact
DxUi pin `1714f8b0c0fa628bcef56de76078f85c869fc50c`. All six local build profiles pass. The three x64 Full runs
each pass 58 ordinary cases (Storage 10, Settings 4, Recovery 5, Editor 39), 23 hidden application captures,
default Hardware and WARP resource/lifecycle checks, and crash/recovery checks. The ASan profile separately
detects the isolated intentional heap-use-after-free probe. The three ARM64 profiles are cross-builds only.

The documentation-only `c60e9ca678ad76460e4b0783622c87099af66ed2` has six fresh build receipts with exactly the
same executable, PDB and applicable ASan runtime bytes. This is explicit reuse of the 334 native execution;
the packet does not retag it as a new c60 runtime. The lead independently rehashes 14 profile artifacts,
1,862 completed payload files and all 69 capture/sidecar pairs, and checks all 174 ordinary case outcomes.

The earlier `d73f423` ASan Full attempt fails Editor case 36/39, the newly added heap-diagnostic oracle. Its
logical 65,536-byte allocation is not represented in the supplied native heap under sanitizer interception.
The corrected oracle independently observes range membership and preserves complete-walk, budget, invalid-handle,
duplicate-handle and exact-boundary checks. A later ASan Full run passes all 39 Editor cases and separately
detects the intentional sanitizer probe. The failed attempt remains retained.

Foreground input, native ARM64 and performance acceptance remain separate gates. The source-bound
[paired performance packet](../performance-c60e9ca/README.md) still has five Original UI regression findings.
RedTilde's [latest PR workflow](https://github.com/RedSalamanders/RedTilde/actions/runs/40204648985) cannot start
its native jobs because of account billing; no hosted ARM64 pass is claimed. The JSON's earlier 334 hosted
entry is explicitly historical. The private receipts, artifacts and unsuccessful attempts are retained locally;
the public projection contains relative source names and digests without machine or account identifiers.
