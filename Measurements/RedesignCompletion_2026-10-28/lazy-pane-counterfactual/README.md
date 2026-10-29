# Rejected lazy optional-pane repair

The fixed 32-process Release scout rejects this repair under its predeclared investment criterion.
Candidate `07191110` creates Files and Settings contents on first use; baseline `e0908338` creates them at startup.
Both use SDK086, the same compiler profile, 58 public headers and nine common performance inputs.

| View | Median working-set decrease | Declining blocks | Command geometric change | Investment gate |
| --- | ---: | ---: | ---: | --- |
| Compose | 268,288 bytes | 4/4 | +1.0764% | Pass |
| Split | 147,456 bytes | 2/4 | +1.7173% | Fail |

Both views required at least 65,536 bytes of median working-set decrease, at least three of four negative
working-set block contrasts, and command change no higher than +5%. Split's signs are negative, positive,
negative, positive. Its lower median does not satisfy the consistency requirement. The route is rejected;
no rerun, study extension, investigation-band change or production adoption follows.

Debug and Release builds and all 49 Editor cases in each profile pass, including two new pane-lifecycle cases.
This is focused functional evidence, not the six-profile qualification that an adopted change would require.
The typed evidence auditor passes 1,635 checks. Independent replay verifies all 32 reports, 288 raw medians,
12 primary effects and their sign-flip/Holm/control decisions, plus 211 retained output/input hashes.
The [projection](projection.json) preserves all nine descriptive metrics per view and every block contrast.
Its schema retains the draft label; it does not claim primary performance acceptance.

Observer coverage is complete: 138 window samples, maximum gap 1.064 seconds. Mean total CPU is 11.49%,
maximum 31.83%; the window is not quiet, and the selected process sampler is not exhaustive.
Four blocks have a minimum two-sided probability of 0.125 before Holm correction. The existing performance
policy remains review-required. The original numerical findings are preserved; the later explicit
[owner memory cost decision](../performance-advice.md) is separate from command/control and policy acceptance.

Private raw data, source and product recovery evidence are retained separately. This public projection contains
aggregate measurements and source/evidence digests; it omits local paths, process/account identifiers and raw environment data.
