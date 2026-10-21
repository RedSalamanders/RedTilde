# S5 noise follow-through — 2026-10-21

Status: revised instrumentation implemented; fixed-scope collection and final validation pending. The application
stays experimental. Hardware, upstream D4c and acceptance of the app's new judge remain separate open gates.

## Investigation and fixed collection scope

The previous [native-v1 packet](../S5_2026-10-21/Final/README.md) retains every noisy control and competing-activity
observation. Its 96 A/A processes all end with 111 threads. The median ratio of the first four versus last sixteen
command samples is 0.9843, and the corresponding complete owner-cycle ratio is 0.9889. Some individual processes
have early spikes, but there is no consistent startup direction; neither a thread-count difference nor universally
insufficient warmup is established. No samples or old rounds are discarded.

The harness performs global thread enumeration around latency collection, often consuming 100–170 ms of process
CPU and perturbing the observation environment. Recovery readiness also differs from worker settlement: ordinary
undo requests immediate cleanup while the worker can still finish asynchronously. These are instrumentation
confounders to isolate, not proven causes of every old flag.

Native-v2 keeps the original two warmup cycles and primary twelve-slot family. It removes global thread inventory
from latency snapshots, reports its unavailable count as null, and waits for successful recovery settlement outside
the timed transactions before observing memory. Resource phases retain full thread inventory and its cost.
Command-only CPU and boundary processor arrays expose additional diagnostics without changing the judge. Normal
recovery remains active during edits. Production callbacks, power policy, process/thread priorities and affinity
are unchanged.

One new frozen baseline retains pre-optimization app callbacks and the identical revised instrumentation/pin.
The prescribed collection is **one A/A then one A/B**, each 12 blocks per Compose/Split view, 64 timed cycles and
two fixed warmups, seed 20261021, with literal schedules and every raw slot retained. Increasing cycles improves
within-process observation support; it is not independent replication beyond the twelve blocks. No repeat is
selected to obtain favorable controls, and no change is claimed relative to the different native-v1 protocol.

The user directed proceeding directly on the shared computer rather than coordinating a quiet lease. Total CPU,
selected process deltas and observation limitations accompany both studies. Actual activity, control stability,
quiet CPU observations and policy acceptance are distinct outcomes. A strong machine does not establish quiet.

## Validation and remaining work

The revised Release Core preflight passes all 25 cases, including the updated native-protocol assertions.
Fifteen observation-tool tests cover operation/chronological medians, boundary changes including processor zero,
legacy parsing, malformed/unsettled v2 data, activity on a lightly loaded powerful host, excessive or missing CPU
observations and partial multi-group coverage. The independent auditor still replays all 6,577 checks in the old
packet. An early empty-array property-access defect was corrected before native collection; validation now passes.

Pending: freeze the matching baseline, collect both studies, inspect every flag and processor/command diagnostic,
independently audit the revised native bytes, run the final source-bound matrix, assess each open gate and preserve
the complete evidence. No existing receipt qualifies revised native code implicitly. The single plan stays WIP.
