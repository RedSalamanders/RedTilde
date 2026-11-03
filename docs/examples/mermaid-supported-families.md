# A gallery of native Mermaid profiles

These eight small examples use the current delivery's bounded native profiles.
The [sequence sample](mermaid-sequence.md) explores the added actor, note and
numbering syntax. These profiles do not claim full Mermaid compatibility. Use F2 to
inspect a fence, then change views or undo an edit without rewriting its source.

## Flowchart

```mermaid
flowchart LR
Draft[Draft] --> Review[Review]
Review --> Saved[Saved]
```

## State diagram

```mermaid
stateDiagram-v2
[*] --> Draft
Draft --> Reviewed : review
Reviewed --> [*]
```

## Class diagram

```mermaid
classDiagram
Document <|-- Markdown
Markdown : +save()
```

## Sequence diagram

```mermaid
sequenceDiagram
participant Writer
participant Notebook
Writer->>Notebook: Save draft
Notebook-->>Writer: Saved
```

## Pie chart

```mermaid
pie showData title Notebook review
"Reviewed" : 60
"Draft" : 40
```

## Entity relationships

```mermaid
erDiagram
NOTEBOOK ||--o{ NOTE : contains
NOTE ||--o{ REVISION : retains
```

## Gantt timeline

```mermaid
gantt
dateFormat YYYY-MM-DD
title Invented notebook review
section Writing
Draft : 2026-11-03, 2d
Review : 2026-11-05, 1d
```

## Mind map

```mermaid
mindmap
  root((Notebook))
    Drafts
      Ideas
    Research
```

All names, dates and counts are invented. Unsupported advanced constructs belong
in the separate fallback sample, rather than being presented as missing families.
