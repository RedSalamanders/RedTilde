# Shared renderer display regressions

These supported cases cover corrected display defects and layouts that must keep
rendering correctly. Every fence should render while headings and neighboring
prose stay ordinary Markdown.

## Styled labels with trailing spaces

```mermaid
flowchart LR
A[<b>Warning: </b>] -->|<i>continue </i>| B[Continue]
```

The style ranges follow the trimmed label. The exact spaces and tags remain in
Source and in the F2 editor; displaying the graph does not rewrite them.

## ER relationships from one entity

```mermaid
erDiagram
CUSTOMER ||--o{ ORDER : places
ORDER ||--|{ LINE-ITEM : contains
CUSTOMER }|..|{ DELIVERY-ADDRESS : uses
```

## Long labels on branches

```mermaid
flowchart TB
Notebook -->|Retain the exact Markdown source| Source
Notebook -->|Prepare a readable preview| Compose
```

The boxes and their connecting corridors should leave room for both labels.

## Dense joins keep separate label lanes

```mermaid
flowchart TD
Draft -->|review| Summary
Notes -->|check| Preview
Draft -->|preview| Preview
Notes -->|summarize| Summary
```

Four relationships cross the same rank. Their labels use separate outside lanes
so they cannot occupy the same midpoint.

## Class multiplicities with two outgoing relationships

```mermaid
classDiagram
Notebook "1" --> "*" Note
Notebook "1" --> "*" Attachment
```

## One marker per unordered task

- [x] The checkbox is the task marker
- [ ] A second ordinary bullet is unnecessary
- Ordinary list items retain their bullet

1. [x] Ordered tasks retain their number

## Readable code labels and line numbers

```cpp
// The language label and line numbers use foreground text ink.
const int observations = 12;
```

Switch between light, dark and high contrast, and verify source/undo after F2.
