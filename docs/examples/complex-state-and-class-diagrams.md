# Complex native state and class diagrams

These flat state machines and colon-member class diagrams target the current
native profiles. Nested states, class blocks, multiplicities and styling are
kept in the separate [fallback examples](rendering-fallback-cases.md).

## S01 — An upload lifecycle with retry and cancellation

```mermaid
stateDiagram-v2
direction LR
state "Waiting for a file" as Idle
state "Preparing local data" as Preparing
state "Sending chunks" as Sending
state "Waiting to retry" as Backoff
state "Checking received data" as Verifying
state "Publishing result" as Publishing
[*] --> Idle
Idle --> Preparing : choose file
Preparing --> Sending : ready
Preparing --> Failed : invalid data
Sending --> Sending : next chunk
Sending --> Backoff : temporary error
Backoff --> Sending : retry
Backoff --> Failed : retry limit
Sending --> Verifying : all chunks sent
Verifying --> Publishing : digest matches
Verifying --> Failed : digest mismatch
Publishing --> Complete : result committed
Publishing --> Failed : publish failed
Preparing --> Cancelled : cancel
Sending --> Cancelled : cancel
Backoff --> Cancelled : cancel
Verifying --> Cancelled : cancel
Failed --> Idle : choose another file
Complete --> [*]
Cancelled --> [*]
Sending : progress is revision-bound
Failed : source and diagnostics retained
```

Check the appended descriptions, self-transition and shared cancellation target.

## S02 — A circuit breaker, top to bottom

```mermaid
stateDiagram
direction TB
[*] --> Closed
Closed --> Closed : successful request
Closed --> Open : failure threshold
Open --> HalfOpen : quiet interval elapsed
HalfOpen --> Closed : probe succeeds
HalfOpen --> Open : probe fails
Closed --> Stopped : shutdown
Open --> Stopped : shutdown
HalfOpen --> Stopped : shutdown
Stopped --> [*]
Closed : ordinary requests allowed
Open : new requests declined
HalfOpen : one bounded probe
```

The cycle and repeated shutdown target should remain identifiable when resized.

## C01 — Revision ownership and derived views

```mermaid
classDiagram
direction LR
class Document
class MarkdownDocument
class Revision
class SourceSnapshot
class Editor
class DerivedView
class OutlineView
class SourceMirror
class EquationInspector
class RecoveryStore
Document : +title
Document : +save(path)
MarkdownDocument : +replaceSource(text)
Revision : +number
Revision : +sourceUnits
SourceSnapshot : +sameRevision(other)
Editor : +applyEdit(range)
Editor : +undo()
DerivedView : +prepare(snapshot)
OutlineView : +navigateToHeading()
SourceMirror : +readOnly
EquationInspector : +notation
RecoveryStore : +queueSnapshot()
Document <|-- MarkdownDocument
DerivedView <|-- OutlineView
DerivedView <|-- SourceMirror
DerivedView <|-- EquationInspector
MarkdownDocument --> Revision : current
SourceSnapshot --> Revision : observes
Editor --> MarkdownDocument : edits
Editor --> SourceSnapshot : publishes
OutlineView --> SourceSnapshot : reads
SourceMirror --> SourceSnapshot : reads
EquationInspector --> SourceSnapshot : reads
RecoveryStore --> SourceSnapshot : checkpoints
```

This is an illustrative object model. Members are plain labels, and relationships
carry no multiplicity or ownership semantics beyond their written labels.

## C02 — Rendering stages and inheritance in the other arrow direction

```mermaid
classDiagram
direction TB
class NativeObject
class EquationObject
class DiagramObject
class Parser
class Layout
class DeviceCache
class RenderTarget
NativeObject : +sourceRange
NativeObject : +prepare()
EquationObject : +notation
DiagramObject : +nodesAndEdges
Parser : +parse(source)
Layout : +measure(width)
DeviceCache : +resourceGeneration
RenderTarget : +drawPrepared()
EquationObject --|> NativeObject
DiagramObject --|> NativeObject
Parser --> NativeObject : creates model
NativeObject --> Layout : supplies model
Layout --> DeviceCache : prepares resources
DeviceCache --> RenderTarget : supplies cached content
Parser -- Layout : shares source identity
```

Both inheritance directions should use hollow triangles with no line through
their interior. Check member line spacing and association-label placement.
