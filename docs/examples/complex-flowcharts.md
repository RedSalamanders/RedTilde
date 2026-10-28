# Complex native flowcharts

These four fixtures target RedTilde's bounded native flowchart profile, without
subgraphs, styling or executable directives. They intentionally combine joins,
cycles and unequal label widths. See the [sample guide](README.md) for checks.

## F01 — Document preparation and recovery, top to bottom

```mermaid
flowchart TD
Start([Open document])
Read[Read UTF-8 bytes]
Validate{Valid source?}
Reject[Keep existing document]
Decode[Decode source]
Revision[Publish immutable revision]
Parse[Analyze Markdown]
Math[Prepare equation leaves]
Graph[Prepare diagram leaves]
Text[Prepare text layout]
Ready{All visible work ready?}
Frame[Complete native frame]
Edited{Source edited?}
Dirty[Mark document changed]
Snapshot[Queue recovery snapshot]
Save{Save requested?}
Stage[Write staged file]
Replace[Replace destination]
Saved([Saved revision])
Failed[Retain dirty source]
Start --> Read --> Validate
Validate -->|no|Reject
Reject --> Start
Validate -->|yes|Decode --> Revision --> Parse
Parse --> Math
Parse --> Graph
Parse --> Text
Math --> Ready
Graph --> Ready
Text --> Ready
Ready -->|pending|Ready
Ready -->|complete|Frame --> Edited
Edited -->|yes|Dirty --> Snapshot
Snapshot --> Parse
Edited -->|no|Save
Save -->|yes|Stage --> Replace
Replace -->|success|Saved
Replace -->|failure|Failed
Failed -->|retry|Stage
Save -->|no|Frame
```

Inspect the readiness self-loop, the recovery back edge and both save outcomes.
The workflow is illustrative; it does not specify the application's internals.

## F02 — Request routing and retries, left to right

```mermaid
flowchart LR
Client([Client request])
Gate{Admitted?}
Denied[Decline request]
Cache{Cached result?}
Hit[Reuse prepared result]
Queue[Join bounded queue]
Worker[Run owned work]
Check{Still current?}
Discard[Discard stale result]
Publish[Publish completion]
Retry{Retry allowed?}
Backoff[Wait for retry]
Error[Report local error]
Done((Complete))
Client --> Gate
Gate -->|no|Denied --> Done
Gate -->|yes|Cache
Cache -->|yes|Hit --> Done
Cache -->|no|Queue --> Worker --> Check
Check -->|yes|Publish --> Done
Check -->|no|Discard --> Done
Worker -->|recoverable error|Retry
Worker -->|permanent error|Error --> Done
Retry -->|yes|Backoff --> Queue
Retry -->|no|Error
Client -->|cancel|Discard
```

Check the exterior retry lane and the two differently labeled edges leaving the
worker. Labels should remain readable over their routes.

## F03 — Grouped endpoints, parallel edges and Unicode, right to left

```mermaid
flowchart RL
Input([Source α and β])
Scan[Scan tokens]
Index[Build line index]
Ready{Ready revision}
Compose[Compose view]
Split[Split view]
Inspect[Read-only inspector]
Output((Presented))
Input --> Scan & Index
Scan & Index --> Ready --> Compose & Split
Ready --> Inspect
Compose -->|first frame|Output
Compose -->|warm frame|Output
Split --> Output
Inspect --> Output
Output -->|new revision|Input
```

The two Compose-to-Output edges are distinct. Grouped endpoints must expand to
all intended connections without dropping or duplicating unrelated edges.

## F04 — Dependency ordering, bottom to top

```mermaid
graph BT
Source[Source revision]
Selection[Selection state]
Theme[Theme and DPI]
Syntax[Parsed syntax]
Metrics[Measured labels]
Geometry[Native geometry]
Resources[Device resources]
Paint[Paint cached content]
Surface([Completed surface])
Source --> Syntax
Syntax --> Geometry
Theme --> Metrics --> Geometry
Theme --> Resources
Geometry --> Paint
Resources --> Paint
Selection --> Paint
Paint --> Surface
Source --- Selection
```

Check the undirected connection and that bottom-to-top orientation does not
reverse arrowheads or exchange node labels. Native routing need not match
Mermaid's browser coordinates.
