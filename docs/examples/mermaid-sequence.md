# A notebook conversation

The native sequence renderer supports actor figures, plain aliases, left/right/over
notes, bare automatic numbering and six message arrow styles. These examples use
invented writing data. Inspect the exact Markdown with F2 or Source, then undo an edit.

## Actors and chronological notes

```mermaid
sequenceDiagram
actor Writer as Notebook author
participant App as Draft editor
autonumber
Note left of Writer: Draft ready
Writer->App: Save draft
Note over Writer,App: One exact document
App-->Writer: Saved
Note right of App: Keep revision
```

Notes stay in the authored timeline. Numbers count messages, starting at one;
notes do not consume a number. Displayed numbers never enter the Markdown source.
Connections and note frames use readable foreground ink in Light, Dark and high contrast.

## Message styles and a self message

```mermaid
sequenceDiagram
actor Writer
participant App
autonumber
Writer->>App: Filled solid head
App-->>Writer: Filled dotted head
Writer-)App: Open solid head
App--)Writer: Open dotted head
Writer->App: Solid without head
App-->Writer: Dotted without head
Writer-)Writer: Continue writing
```

## A default flow direction

```mermaid
flowchart
Draft --> Review
Review --> Saved
```

Without a direction token, `flowchart` and `graph` use top-to-bottom placement.
Loops, activation bars, creation/destruction, custom numbering arguments and
interaction directives remain outside this bounded profile and preserve their
complete editable fence. They are not represented by a partial native diagram.
