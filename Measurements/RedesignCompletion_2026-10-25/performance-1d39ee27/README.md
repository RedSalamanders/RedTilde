# RedTilde 1d39ee27 performance projection

Source-bound performance packet. Candidate `1d39ee27ee63aedb01197b230f399963e0caad9f` uses DxUi `2fed82465dcee9f213753754e0e8e54f353cf38f`; the three references remain separate: original UI (`c1579053d381faa03d945a448bad9f699cd30fd8`), starting redesign (`4ce958a8090cb673b39f3a89b841451c27acab53`), and actual PR14 base (`661858a26013a6a1b7a36e991a31e656622045f6`). Each reference received only the shared build/protocol overlay and was frozen at x64 Release with SDK2fed. This packet is descriptive evidence, not a performance acceptance decision.

## Study integrity

The collection preserves 576/576 raw native report hashes. Independent reconstruction produced 72 metric rows, 864 block effects and 1,728 same-side control drifts. Each cohort has separate AA same-binary controls and AB candidate comparisons, with 12 blocks per view and 64 cycles per native report. The 13-column CSV contains 72 rows (936 data cells); every CSV value was replayed against the JSON projection.

| Cohort | Phase | Rows | Regressions | Memory improvements | Stable controls | Unstable controls |
|---|---:|---:|---:|---:|---:|---:|
| original-ui | AA | 12 | 0 | 0 | 6 | 6 |
| original-ui | AB | 12 | 4 | 0 | 6 | 6 |
| starting-redesign | AA | 12 | 0 | 0 | 6 | 6 |
| starting-redesign | AB | 12 | 0 | 4 | 6 | 6 |
| actual-pr14-base | AA | 12 | 0 | 0 | 6 | 6 |
| actual-pr14-base | AB | 12 | 0 | 0 | 6 | 6 |

The original-UI AB comparison has four regression findings: Compose command time +10.8579%, Split command time +7.1672%, Split owner CPU cycles +12.3126%, and Split working set +2.0204%. Private bytes were within the investigation band. These are four outcomes from that cohort/phase only.

Starting-redesign AB has four resolved memory improvements and eight no-resolved-change rows. Actual PR14-base AB has 12 no-resolved-change rows. None of these comparisons is pooled with another cohort; the starting redesign and PR14-base do not replace the original-UI reference.

## Observer and qualification limits

All six observer envelopes report `quietCpuWindowVerified=false`. The CPU observer records system busy intervals and selected external-process activity, but selected names are not exhaustive and process start/exit work can escape interval deltas. GPU activity and physical occlusion were not measured. No observer result identifies a cause for a performance change.

ARM64 evidence is cross-build only. The x64 native matrix and keyboard interaction receipt are separate qualification records. The interactive receipt covers nine passed cases and 168 injected keyboard events; foreground and focus were unchanged, the cursor was restored, and the clipboard was untouched. Physical IME/assistive-technology input qualification is not claimed. Neither record closes hosted CI or this review-required performance analysis. The report provides descriptive local comparisons and does not authorize a policy or acceptance conclusion.

Files in this packet contain the public-safe JSON summary and CSV projection plus an independent audit record. They omit raw native payloads, report filenames, absolute paths, PIDs and screen coordinates.
