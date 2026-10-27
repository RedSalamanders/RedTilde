# Intentional math and diagram fallback cases

Every numbered case below is deliberately outside the current native grammar or
malformed. Expected behavior is exact editable literal source with diagnostics,
not a partly rendered approximation. This document is a negative-test specimen,
not a supported-feature demonstration. No package, directive or command executes.

Compare with [supported mathematics](complex-math.md), [flowcharts](complex-flowcharts.md)
and [state/class diagrams](complex-state-and-class-diagrams.md).

## X01 — Unsupported array environment

$$\begin{array}{rcl}x+y&=&3\\x-y&=&1\end{array}$$

Use the supported `aligned` or matrix environments in ordinary documents.

## X02 — Unsupported user-defined macro

$$\newcommand{\sample}[1]{\frac{#1}{2}}\sample{x}$$

## X03 — Unsupported styling command

$$\color{red}{x^2+y^2}$$

## X04 — Unequal matrix column counts

$$\begin{bmatrix}a&b&c\\d&e\end{bmatrix}$$

## X05 — Unmatched scalable delimiter

$$\left(\frac{a+b}{c+d}$$

## X06 — Missing argument and duplicate script

$$\frac{a}$$

$$x_i_j$$

## X07 — Unsupported sequence diagram

```mermaid
sequenceDiagram
participant Writer
participant Editor
participant Worker
participant Storage
participant Recovery
Writer->>Editor: Change source
Editor->>Worker: Prepare immutable revision
loop Each visible object
    Worker-->>Editor: Publish prepared result
end
par Save document
    Editor->>Storage: Write staged bytes
    alt Replacement succeeds
        Storage-->>Editor: Saved revision
    else Destination unavailable
        Storage-->>Editor: Retain dirty state
    end
and Retain recovery
    Editor->>Recovery: Queue revision snapshot
    Recovery-->>Editor: Checkpoint complete
end
Editor-->>Writer: Show current revision state
```

## X08 — Unsupported flowchart subgraph

```mermaid
flowchart TD
subgraph Preparation
Parse[Parse source] --> Layout[Measure geometry]
end
Layout --> Paint[Paint content]
```

## X09 — Unsupported composite state

```mermaid
stateDiagram-v2
[*] --> Editing
state Editing {
    [*] --> Clean
    Clean --> Dirty : type
    Dirty --> Clean : save
}
Editing --> [*]
```

## X10 — Unsupported class blocks and multiplicities

```mermaid
classDiagram
class Notebook {
    +title
    +open()
}
Notebook "1" --> "many" Page
```

## X11 — Unsupported styling directive

```mermaid
flowchart LR
A[Source] --> B[Result]
style A fill:#ffdddd,stroke:#aa0000
```

## X12 — Malformed edge, followed by a supported neighbor

```mermaid
flowchart TD
Ready -->
```

The supported neighbor below should still render independently:

$$\frac{1}{1+x^2}+\sqrt{1+y^2}$$

```mermaid
flowchart LR
Source[Exact source] --> Ready([Ready to edit])
```

Use F2, cancel, then edit and undo one rejected case. Preserve its complete
envelope and verify that the neighboring equation and graph remain independent.
