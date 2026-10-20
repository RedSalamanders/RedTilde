# Common services

Read the root guidance and owning Core/Editor contract. Keep services independent of UI controls except public
immutable DxUi value types. Storage validates all input before changing a live session. Return errors explicitly;
tests use unique directories under the harness output. Workers own snapshots, never mutable controls or HWND
lifetime. Crash-path code has separately documented allocation restrictions; ordinary code uses WIL.
