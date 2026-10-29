# Rejected initial-frame batching ablation

The fixed four-process resource check and 32-process Release scout reject disabling initial-root-child batching
as this memory repair. Candidate `8d0fdee2` changes that single application option from true to false;
baseline `e0908338` retains it. Both preserve current UI/features, SDK086, the compiler profile,
58 public headers and nine common performance inputs. The private Editor profile requires zero initial flushes;
the shipping profile's positive-flush expectation is unchanged.

| View | Median working-set decrease | Declining blocks | Command geometric change | Median private-byte increase |
| --- | ---: | ---: | ---: | ---: |
| Compose | 2,048 bytes | 1/4 | −1.2150% | 5,113,856 bytes |
| Split | 65,536 bytes | 1/4 | −1.4591% | 3,934,208 bytes |

Both views required at least 65,536 bytes of median working-set decrease, three of four declining blocks and
command change no higher than +5%. Both fail consistency; Compose also fails the minimum decrease.
Private-byte block contrasts are positive in all four blocks of both views. The route is rejected without
reruns, extensions, changed investigation bands or production adoption. This does not identify the historical
4 MiB allocation or establish an accepted statistical regression or benefit.

The singleton resource observations retain each operation separately:

| View and operation | Batching on, wall µs | Batching off, wall µs | Off minus on, wall µs | On, owner cycles | Off, owner cycles |
| --- | ---: | ---: | ---: | ---: | ---: |
| Compose first completion | 223,315.1 | 198,811.0 | −24,504.1 | 395,222,963 | 358,023,124 |
| Compose resize | 5,872.5 | 9,357.8 | +3,485.3 | 9,662,236 | 9,070,423 |
| Compose recreation 0 | 71,333.5 | 72,027.0 | +693.5 | 102,230,725 | 99,333,131 |
| Compose recreation 1 | 82,736.7 | 73,411.0 | −9,325.7 | 108,367,219 | 104,184,009 |
| Compose recreation 2 | 72,076.4 | 73,796.9 | +1,720.5 | 99,909,981 | 100,059,683 |
| Split first completion | 213,900.9 | 206,680.0 | −7,220.9 | 378,093,698 | 372,978,338 |
| Split resize | 8,921.9 | 10,358.6 | +1,436.7 | 17,069,593 | 16,656,820 |
| Split recreation 0 | 77,501.5 | 76,154.5 | −1,347.0 | 104,012,155 | 105,946,475 |
| Split recreation 1 | 89,394.6 | 99,454.6 | +10,060.0 | 111,900,746 | 117,737,832 |
| Split recreation 2 | 82,697.1 | 76,204.0 | −6,493.1 | 111,383,386 | 109,671,216 |

Initial flush counts are seven in Compose and six in Split with batching on, and zero with it off.
First-completion windows start after application/UI/theme/view/viewport setup. Pre-loop cumulative process CPU
includes earlier setup and collection costs and has timer granularity. These are hidden completed operations,
not startup-to-visible or presentation latency. Resize and recreation costs are mixed and are not averaged away.

The canonical Release build, all 47 Editor cases, full validators and formatting pass for this private profile.
All 36 resource phases and 44 snapshots satisfy their collection/resource bounds; 20 PNGs validate and twelve
device-replacement pairs are byte-exact. Native source, undo, caret, geometry, idle and teardown guards pass.
These checks do not claim cross-role pixel equality or full six-profile qualification.

The typed evidence auditor passes 1,635 checks. Independent replay verifies 32 raw reports, 288 medians,
all twelve primary effects/probabilities/Holm/control decisions and 253 bound input/output hashes.
Observer coverage is complete at 155 window samples with maximum gap 1.055 seconds; mean CPU is 11.19%
and maximum 26.54%, so quiet-host acceptance is unproved. Four blocks have minimum two-sided probability
0.125 before Holm correction. The canonical result remains review-required with no resolved change.

The [projection](projection.json) retains all nine metrics per view, block signs, individual resource costs and
all 44 snapshot rows. Its schema keeps the draft label; publication does not promote it to primary acceptance.
Raw evidence and source/product recovery stay private. The public projection omits local paths,
process/account identifiers and raw environment values. The later explicit [owner memory cost decision](../performance-advice.md)
does not change this rejection or close the remaining command/control, policy, CI or hosted ARM64 gates.
