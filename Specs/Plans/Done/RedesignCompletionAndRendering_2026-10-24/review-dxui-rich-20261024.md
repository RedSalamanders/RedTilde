# DxUi rich-text review ledger - 2026-10-24

Read-only supplement for the 112 assigned production/test inventory rows. This agent did not run tests/builds. Parent-run diagnostic evidence is identified as parent-run and paths are preserved. Other five build/tooling/data rows remain with `review_ledger`.

```json
{
  "reviewedCommit": "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
  "sourceCommit": "cd69aa75d0b8d1c26de84996936eed0be316d8f5",
  "scope": "DxUi rich-text/Markdown/math/flowchart/semantic/prepared A0 supplement; 112 paths reconcile to coordinator inventory. Reviewed rows have full current-file reads recorded; pending rows include paths modified after the reviewed snapshot or not yet read. Other five build/tooling/data rows remain with review_ledger. No tests/builds run by this agent.",
  "files": [
    {
      "repository": "DxUi",
      "path": "include/DxUi/MarkdownSyntax.h",
      "sha256": "9417156cf96e3d7dd80b77dba33cd9e334dc4adfcd6851b8bdd28cb4b06da4e8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks",
        "Support.Tests.MarkdownInlines",
        "Support.Tests.MarkdownCapacity"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "MarkdownParser is the only constructor/publication entry and SemanticBuilder consumes immutable syntax.",
        "ownershipAndLifetime": "Syntax owns source, revision, dialect, lexical/block/inline trees, footnotes and diagnostics; all indices are revision-local.",
        "failureAndCancellation": "Begin contract preserves prior request on failure; Advance discards staged request on failure; caps and scalar/source coordinates explicit.",
        "testCoverage": "Support.Tests.MarkdownBlocks, MarkdownInlines, MarkdownCapacity and corpus tests cover parsing; exact checks are attributed on their test rows."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/PreparedTreeAccessibility.h",
      "sha256": "ccec7113bfe2aa93e51d4beb89e2572f7c15c2990a653d68f87d759ee7664b89",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.PreparedTreeAccessibilityFixture",
        "Embedded.PreparedTreeAccessibilityRowsFixture"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Tree UIA host adapters consume IPreparedTreeAccessibilityRows; accessibility snapshots retain shared source owner.",
        "ownershipAndLifetime": "Borrowed text view lifetime is supplied by retained immutable source owner; interface is data-only/no COM/UI ownership.",
        "failureAndCancellation": "Count/GetItem/FindItem are noexcept bounded reads; documentation requires epoch replacement when row semantics change.",
        "testCoverage": "Prepared-tree accessibility embedded fixtures cover snapshot lifetime/epoch; no standalone interface-only test."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichDocument.h",
      "sha256": "567884f5dbc1caa7895736f073b10b9c1ec15f1c5c6f5eac227a17d50fb2e359",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument",
        "Embedded.RichDocumentStorageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Document is source/model base for parser, semantic transactions and editor; snapshots feed background read-only parsing.",
        "ownershipAndLifetime": "Immutable snapshots share piece-tree revision state; document owns selection and bounded undo/redo history.",
        "failureAndCancellation": "UTF-16 scalar/CRLF boundaries and 64 Ki-unit/history caps guard replacement; transient composition changes retain/restore history semantics.",
        "testCoverage": "Controls.Tests.RichDocument and rich-document storage/failure fixtures cover pieces/history; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichImage.h",
      "sha256": "2641ebf4c5d2d053f87de96a3d2584bb9e9a0f274a5b705ac113a297a5bec4cc",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for RichImage.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for RichImage.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMarkdown.h",
      "sha256": "de050951881f6de69c9e3c21a933720495ea6b6134eeadb21381476f06412ae1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown.RunRichMarkdownTests"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Legacy projection path backs v0 layout while semantic layout is opt-in; ToggleMarker edits Document source.",
        "ownershipAndLifetime": "Projection owns source/display/maps/runs; immutable block projection payloads share line outputs across preparations.",
        "failureAndCancellation": "PreparedProjection stages changed blocks/indexes before swap; API explicitly disclaims CommonMark conformance.",
        "testCoverage": "Controls.Tests.RichMarkdown exercises projection, incremental reuse and mapping; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichMath.h",
      "sha256": "7ca8efde3a6e3d255770c507dcdf649f9e5ce2fd58ba32cca45297dafb60a5e0",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Public math interface exposes typed model/result and bounded options.",
        "ownershipAndLifetime": "Result ownership is value-based; caller retains source independently.",
        "failureAndCancellation": "Failure status distinguishes unsupported/error from ready output.",
        "testCoverage": "Support.Tests.MathCorpus exercises public contract cases. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemantic.h",
      "sha256": "f5f24b5634463f39f564bc02986fdeeda2bbbbb3e69f64e2a937cf18186e29a9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "RichSemanticDocument uses SourceDisplayMap; semantic commands and editor map display selections back to source.",
        "ownershipAndLifetime": "Immutable map owns source/display spans and affinity indexes; SourceEdit replacement views are synchronous borrowed values.",
        "failureAndCancellation": "Build validates partition, ownership and UTF-16; ApplySourceEdits validates revision/ranges and stages before Document::Replace.",
        "testCoverage": "Support.Tests.RichSemanticDocument and command/source-change fixtures cover mapping and edits; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichSemanticDocument.h",
      "sha256": "382e2e063531b4b86a624a6d8208c65b589799a44e6eb782a6480e86f5b705d1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.SemanticDiagrams",
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "SemanticBuilder consumes MarkdownSyntax and publishes immutable SemanticDocument consumed by geometry, resources and UIA.",
        "ownershipAndLifetime": "Document owns syntax, map, elements/paragraphs/navigation, style runs and value-only extension model; no SDK/host resource.",
        "failureAndCancellation": "Begin/Advance staging retains previous publication on failure; source editing is only a revision-local eligibility hint guarded by owner at action time.",
        "testCoverage": "Support.Tests.RichSemanticDocument, semantic diagrams, image/math and accessibility fixtures; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextCommands.h",
      "sha256": "1770a9b2b492e85ab9558c8adae4e1e4e333b05592cc4bd2f21efdeb3c446620",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "Editor routes MarkdownCommand requests to BuildMarkdownCommand/Deletion and ApplyMarkdownCommand.",
        "ownershipAndLifetime": "Request views are borrowed only while planner builds owned immutable edits/source snapshot and expected revision.",
        "failureAndCancellation": "Rejected plan leaves Document intact; Apply validates expected source/revision before one transaction.",
        "testCoverage": "Support.Tests.RichSemanticEditor structural command source/selection/history cases; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextLayout.h",
      "sha256": "8a23fbec041a2d2b0e90dcca19ff77eaff1725697d67868f20683b92f32163f8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout and PreparedSemanticGeometry use PreparedLayout for text geometry and draw; facade handles v0/semantic selection.",
        "ownershipAndLifetime": "Caller supplies borrowed DirectWrite factory/targets/brushes; prepared state owns layouts/maps but no HWND/device/thread.",
        "failureAndCancellation": "Validation and bounded work APIs document failed preparation preserves prior pair; draw borrows transient brushes and restores shared layout state.",
        "testCoverage": "Controls.Tests.RichTextLayout and RichSemanticGeometry cover mapping/layout/geometry; no run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "include/DxUi/RichTextPresentation.h",
      "sha256": "d96715b8b40f75bbd31e4d4a00b505cd8e6fac92db12da65b925ef2b948652e0",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry.Admission"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found at this public API boundary.",
      "reviewNotes": {
        "callers": "SemanticSpacing is consumed by semantic geometry/layout options.",
        "ownershipAndLifetime": "Value-only fixed-size arrays/scalar; no resources or lifetime dependencies.",
        "failureAndCancellation": "IsValid rejects non-finite/out-of-policy spacing before geometry preparation.",
        "testCoverage": "Semantic geometry admission tests cover invalid/valid spacing; no dedicated struct test found."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownBlocks.h",
      "sha256": "3c9c206a2acd05358cc281a26e75dbc5ce2d3b3fc743192d72b79ac8fcc6c5ad",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Block dispatch recognizes bounded block starts and advances source intervals monotonically.",
        "ownershipAndLifetime": "Nested block parsing uses explicit stacks and capped node/depth counts.",
        "failureAndCancellation": "Unsupported constructs preserve source slices and diagnostics rather than publishing partial semantic output.",
        "testCoverage": "Support.Tests.MarkdownBlocks covers fenced blocks, lists, tables and corpus extraction. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownCharacterReference.h",
      "sha256": "4fb5f5eac34311d605f9916f2889f13f26a5dae00b02799b7813eae1c240f265",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownCharacterReference.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownCharacterReference.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownEntityData.h",
      "sha256": "fe27d6f6fd7bdb89dd252729524aa835a0ea798087e409a81bcaa4e255ecaf33",
      "category": "production",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Generated entity table is immutable lookup data used by character-reference decoding.",
        "ownershipAndLifetime": "Lookup returns spans into static storage without per-call allocation or ownership transfer.",
        "failureAndCancellation": "Parser validates complete names and scalar values before publication.",
        "testCoverage": "Support.Tests.MarkdownBlocks includes named/numeric references and invalid forms. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownInlines.h",
      "sha256": "62cce330ebfdb9e65bbd337ca5a521854c8b73b4111911669a194492151c0487",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Inline parser tracks source spans for emphasis/code/link/image constructs.",
        "ownershipAndLifetime": "Inline nodes own decoded text and preserve source correlation for untouched syntax.",
        "failureAndCancellation": "Depth, source and node limits fail closed with exact source retention.",
        "testCoverage": "Support.Tests.MarkdownInlines covers nested delimiters, entities, URLs and malformed syntax. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownLabels.h",
      "sha256": "13031872578ddb54e53efe212a337baefeb195074f23b6e783b45735c6f2682c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownLabels.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownLabels.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSourceCorrelation.h",
      "sha256": "a7af9b55d38cb7691fb4ce2469b0d2a3c8b00d4f7c8a667b21f4c1ab3c3f4b1c",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.MarkdownSourceCorrelation.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.MarkdownSourceCorrelation.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownSyntax.cpp",
      "sha256": "735e314ec52cd8b2d44c55c704254c387f822a2bb259c57ac845fb0a5074414a",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Parse entry validates options and stages the request before replacing published syntax.",
        "ownershipAndLifetime": "Incremental Advance resumes bounded parsing and retains the prior committed tree on failure.",
        "failureAndCancellation": "Revision and source intervals propagate across block and inline parsing.",
        "testCoverage": "Support.Tests.MarkdownBlocks and MarkdownCapacity cover parser output and budgets. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownTableCells.h",
      "sha256": "f548d2132cf17c931d8a894fc9a6354d39e6ff17cd9291bec8ea8c8d929812da",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Cell splitting respects escaped pipes and normalizes syntax padding.",
        "ownershipAndLifetime": "Cells are returned through bounded spans without allocating.",
        "failureAndCancellation": "Invalid row forms fail as a whole instead of publishing incomplete cells.",
        "testCoverage": "Support.Tests.MarkdownBlocks includes empty-cell and table-row cases. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.MarkdownUnicodeData.h",
      "sha256": "1e49ee2858300241526b6a257b44aa0c45d90b6ed35c43c900403aa449bca262",
      "category": "production",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this bounded pass.",
      "reviewNotes": {
        "callers": "Generated Unicode property data is static and queried through bounded searches.",
        "ownershipAndLifetime": "No ownership or dynamic initialization is introduced by lookups.",
        "failureAndCancellation": "Callers validate scalar ranges before querying grapheme properties.",
        "testCoverage": "Support.Tests.MarkdownInlines and RichTextLayout exercise Unicode behavior. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.cpp",
      "sha256": "dfe55c6745aeb5a7924f09a392dd2af994d12f624d961e329fa89a9ff2cbd433",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "CheckOriginalCorpus parses per-example outcomes; CheckFamilyGrammar covers family isolation and diagnostics.",
        "ownershipAndLifetime": "Parser preserves node/edge source intervals and publishes only a complete model.",
        "failureAndCancellation": "CheckFanout covers stable Cartesian expansion and atomic caps.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchart.h",
      "sha256": "2358fe2921aada2cdd0f6af0c11bc8044b86c32d14faf6584e42bce77c6f74af",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "Model carries family, direction, shape and typed relation.",
        "ownershipAndLifetime": "Diagnostic records outcome, issue category and source interval.",
        "failureAndCancellation": "Model vectors are bounded and parser publication is all-or-nothing.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.cpp",
      "sha256": "d0955bda79392340748bf7abeb22d989c1ac9db141b8a3e38597ff5d166aca66",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Corpus lays out every admitted original graph.",
        "ownershipAndLifetime": "Geometry checks route, ports and extents.",
        "failureAndCancellation": "Fanout checks finite paths and endpoint ports.",
        "testCoverage": "Stadium sizing and ports use capsule geometry; global lane width remains shared. Tests referenced as coverage only; this agent did not run them."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeFlowchartLayout.h",
      "sha256": "1ee37f869cf3935677b26a09638cf7347cf1f5b80740ae51fc5a69638c92575b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Layout accepts bounded parsed graphs.",
        "ownershipAndLifetime": "Plan stores bounded node and edge placements.",
        "failureAndCancellation": "Shape and relation types survive through placed geometry.",
        "testCoverage": "Maximum extents and work steps are explicit. Tests referenced as coverage only; this agent did not run them."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.NativeMath.cpp",
      "sha256": "eae4ea21cb0c020f60d641f46d08478f342cef0a34bfd4f081de9ce1059cd6a9",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Native math parser is reached through semantic native extension dispatch and prepared-math construction.",
        "ownershipAndLifetime": "Model owns parsed TeX source/token data and prepared output; no raw COM ownership.",
        "failureAndCancellation": "Unsupported syntax and explicit caps produce failure while the caller retains exact fence source.",
        "testCoverage": "Support.Tests.MathCorpus contains normalized parser expectations and adversarial limits. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.cpp",
      "sha256": "dad35e17dd95b0c29f3f2abbcfaa928e1acd42f868ee7e08a257e1d14bc1e26d",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Relations exercises typed inheritance paint.",
        "ownershipAndLifetime": "StadiumRaster checks capsule rendering against a nonrectangular oracle.",
        "failureAndCancellation": "Labels exercises measured text geometry.",
        "testCoverage": "Incremental preparation publishes after layout and paths are complete. Tests referenced as coverage only; this agent did not run them."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedFlowchart.h",
      "sha256": "b9469918cdb2068d4cdb65f6d4ed0308eea258456360b2b072732d508d6f44fb",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Prepared renderer validates model limits and resource epoch.",
        "ownershipAndLifetime": "Geometry/readiness are exposed as immutable prepared state.",
        "failureAndCancellation": "Drawing keeps relation arrow and node shape paths distinct.",
        "testCoverage": "Rendering uses native paths without runtime dependency loading. Tests referenced as coverage only; this agent did not run them."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.PreparedMath.h",
      "sha256": "20035b7ce1c0bb7da3ecaeb3261177d3cb2d949c68e399dffcabf4b118d06084",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "PreparedMath stores immutable layout/raster data and resource epoch.",
        "ownershipAndLifetime": "Preparation is bounded and publishes complete output only after measurement/render succeeds.",
        "failureAndCancellation": "Resource recreation invalidates stale prepared output without losing source fallback.",
        "testCoverage": "Support.Tests.MathCorpus and Embedded.Tests.RichTextResources exercise preparation/rendering. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibility.h",
      "sha256": "5ec593951306aab44bfd529af2e3e3928965354f64af271e0c3b019662235aea",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility semantic provider properties/navigation/actions"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "UIA providers query guarded snapshots and dispatch mutations to the window thread; embedded providers enforce their own thread contract.",
        "ownershipAndLifetime": "Provider retains target ref, path/identity and weak document identity; every call revalidates snapshot, binding, semantic revision and element bounds.",
        "failureAndCancellation": "Stale/wrong-role records return UIA unavailable/unsupported; Toggle/Invoke translate bad_alloc and std::exception at noexcept COM boundaries.",
        "testCoverage": "Controls.Tests.RichTextAccessibility semantic-provider fixtures cover properties, navigation, actions and embedded/client providers; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichAccessibilityEpoch.h",
      "sha256": "65b95f8d202c883d755aadd018f81467912740a0f89213635aaabc4364d620e6",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility stale-provider lifecycle"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument and host accessibility snapshots use the epoch to invalidate stale providers.",
        "ownershipAndLifetime": "Data-only object owns one atomic revision, no UI, HWND, host or SDK reference.",
        "failureAndCancellation": "Revision invalidates provider identity without dereferencing the editor from foreign UIA calls.",
        "testCoverage": "Controls.Tests.RichTextAccessibility lifecycle fixtures; no standalone epoch wraparound case found; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocument.cpp",
      "sha256": "c016485231a4bc39f53415159fb8a3a8e6e80488738c1910ffdbd88c88e9b271",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.RichDocument.cpp in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.RichDocument.cpp to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichDocumentLayout.cpp",
      "sha256": "f03cd7aade94d6a8d88725dad7158f14335ff65e6ff83dea22069dd87f58c5d4",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for DxUi.RichDocumentLayout.cpp in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for DxUi.RichDocumentLayout.cpp to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichEditorLayout.h",
      "sha256": "fe83b32163b16431015a56ba7849b280f67c07291271fbc6ad3cb33ca4c96937",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout.RunRichTextLayoutTests",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichTextEditor begins/advances a facade for legacy v0 or prepared semantic geometry.",
        "ownershipAndLifetime": "Facade switches after target prep completes; it cancels other staging path and publishes generation changes at completion.",
        "failureAndCancellation": "Queries dispatch to active layout; Clear retires both. Caller owns exception recovery.",
        "testCoverage": "Controls.Tests.RichTextLayout and semantic geometry fixtures cover projection/layout handoff; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.cpp",
      "sha256": "84047cf4d3f8765f03d7564561405082d751bd4b7387a8653adfc4fdec60244d",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage.Files",
        "Support.Tests.RichImage.Core",
        "Embedded.Tests.RichImageFailures"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImage LoadFile/Decode/CreatePbgra are invoked by image service from RichImageResources requests.",
        "ownershipAndLifetime": "Decode owns bytes while lazy WIC sources live, validates format/size and stages pixels before assigning output.",
        "failureAndCancellation": "Cancellation polled through file/WIC/pixel loops; HRESULT failures preserve output; exceptions translate at noexcept APIs.",
        "testCoverage": "Support.Tests.RichImage.Files/Core and embedded image failure/capture fixtures cover decode/orientation/failures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImage.h",
      "sha256": "34d27813d652032e61f48accb25719fbef6369943513d8a9bd07a4fb2274ebcd",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage",
        "Support.Tests.RichImageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImageResources and image service share request state and diagnostic file-call seam.",
        "ownershipAndLifetime": "Requests own path, identity, kind and atomic cancellation; test injection is diagnostic-build only.",
        "failureAndCancellation": "Cancellation token is read-only to workers; request state follows shared ownership.",
        "testCoverage": "Support.Tests.RichImage and Support.Tests.RichImageResources cover request/file lifetimes; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImagePaths.h",
      "sha256": "864e905b5674f26b6d3da143117312eadb0524a278a1a25b67832f7b0e5bf5e1",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImagePaths.Paths",
        "Support.Tests.RichImagePaths.FileReferences",
        "Support.Tests.RichImagePaths.Retained"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichImageResources resolves destinations against document base; paste uses BuildMarkdownFileReference.",
        "ownershipAndLifetime": "Lexical parser is bounded and owns normalized text/component spans; no filesystem lookup or network access.",
        "failureAndCancellation": "Local resolver rejects ambiguous roots/scheme encodings and invalid UTF-16; remote URI handling is lexical.",
        "testCoverage": "Support.Tests.RichImagePaths.Paths/FileReferences/Retained plus codec path fixtures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichImageResources.h",
      "sha256": "a658daf91c20acff83cf3c44ca89762fb1467b408384c438321b46adfaa5e490",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImageResources"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "RichTextEditor routes prepared semantic image elements to owner-thread RichImageResources; application service owns asynchronous worker jobs.",
        "ownershipAndLifetime": "Resource entries hold shared image/request state; cancellation is atomic and callbacks/workers are explicitly caller-owned.",
        "failureAndCancellation": "Begin/Advance stage bounded source and placement sets; Complete only admits dispatched current requests and enforces decoded-byte caps.",
        "testCoverage": "Support.Tests.RichImageResources and Controls.Tests.RichTextInput cover deduplication, path policy, cancellation, queue overflow and failed fallback. Test attribution only; this agent did not run it."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichMarkdownProjection.cpp",
      "sha256": "15df5d3c3ecdd6faf431cd2a54fede054a6fe643cc09c9c090cc8a63b0900a97",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown.RunRichMarkdownTests",
        "Controls.Tests.RichDocumentLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout consumes projection blocks as v0 compatibility; RichEditorLayout chooses v0 when semantic prep absent.",
        "ownershipAndLifetime": "PreparedProjection stages changed physical-line blocks, reuses immutable neighbors and publishes complete rebuilt mappings.",
        "failureAndCancellation": "Mapping is range-checked; coverage mismatch throws before state swap.",
        "testCoverage": "Controls.Tests.RichMarkdown and rich-document layout fixtures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemantic.cpp",
      "sha256": "b90e450504c76b5d12808b46d796419ee4dd747817b42e70d3d946a75a5cd209",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument",
        "Support.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument builds SourceDisplayMap; ApplySourceEdits applies source-owned changes.",
        "ownershipAndLifetime": "Map owns source/display/spans and affinity anchors; primary source ownership is checked for overlap.",
        "failureAndCancellation": "Invalid ranges/affinity/spans/UTF-16/transactions fail before publication; edits use Document::Replace history.",
        "testCoverage": "Support.Tests.RichSemanticDocument/source-change fixtures cover maps and transactions; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticDocument.cpp",
      "sha256": "583ca51586e3e84d7508d43fb157f262efcf95f83a5c7d50e3a359914f1631b3",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Document builder consumes immutable RichSemantic input and publishes staged document state.",
        "ownershipAndLifetime": "Source identity/revision and object sidecars are owned through document storage.",
        "failureAndCancellation": "Native preparation errors propagate as HRESULT/status so fallback retains source.",
        "testCoverage": "Support.Tests.RichSemanticDocument and SemanticSourceBlock cover projection/failure paths. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.cpp",
      "sha256": "0d161cdc926b0d2ab6c2f0eae52c7d1645f57fa01aea53562f93b5ecde1e7530",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Geometry preparation traverses semantic nodes and native prepared object references.",
        "ownershipAndLifetime": "Plans keep source intervals, bounds and object handles scoped to the document revision.",
        "failureAndCancellation": "Invalid geometry and allocation failures leave publication staged; caller owns fallback policy.",
        "testCoverage": "Support.Tests.RichSemanticGeometry and PreparedFlowchart cover layout and failure oracles. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticGeometry.h",
      "sha256": "63ffba706d7b833cd7dd5874dbc5ac5bbc7935c595c842e481073b6869df120b",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Geometry API separates immutable plan data from preparation progress.",
        "ownershipAndLifetime": "Placed objects and source ranges are revision-local.",
        "failureAndCancellation": "Budgets and HRESULT-bearing preparation states keep failures explicit.",
        "testCoverage": "Support.Tests.RichSemanticGeometry and SemanticSourceBlock exercise consuming contracts. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticHtml.h",
      "sha256": "2c8417f26eca42b5cc0eac43d89848eaccdc3b6ae37f3f756892ad4936f1c80e",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticSourceBlock",
        "Support.Tests.RichSemanticDocument HTML fixtures"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument calls NativeHtmlPlan while normalizing inline and block leaves.",
        "ownershipAndLifetime": "Tag vectors, nesting stacks, source offsets and title/destination values stage per leaf; admission is bounded.",
        "failureAndCancellation": "Unsupported/crossed/unclosed tags or overlapping anchors remain literal; no general HTML recovery/execution.",
        "testCoverage": "Support.Tests.SemanticSourceBlock and semantic-document HTML fixtures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSemanticValues.h",
      "sha256": "3031f1706c7c011ccf049794586cc94efe05f666dd1aed33726d2619102a6861",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticDocument entity and code ownership"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichSemanticDocument resolves inline nodes through SemanticValue before emitting display/source spans.",
        "ownershipAndLifetime": "Units own source ranges and generated/transformed flags; entities decode only as complete lexical tokens.",
        "failureAndCancellation": "Mismatch throws instead of guessing source coordinates; output remains staged.",
        "testCoverage": "Support.Tests.RichSemanticDocument and inline/model corpus entity/code fixtures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichSourceInk.h",
      "sha256": "829529fc34f2c1e4a565a398b85ec3b7154caa2585c170066af329e9cf0a9343",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownInlines; embedded source rendering case name pending"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "RichTextEditor advances RichSourceInk for source coloring and passes completed ranges to PreparedLayout.",
        "ownershipAndLifetime": "256-step turn budget advances node/inline scans, merge order and event publication; incomplete ranges hidden.",
        "failureAndCancellation": "Overflow gives ordinary source; exceptions clear partial state and propagate.",
        "testCoverage": "Markdown inline and embedded rendering fixtures cover coloring/progress; exact focused case names need test-file review."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextCommands.cpp",
      "sha256": "6c26a98f0b5187969d3e5a0ae74f0cbde2ba349868a9e3bc5467aec3fc3bb227",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Command dispatch resolves selections and source units before structural mutation.",
        "ownershipAndLifetime": "Undo transaction and source snapshots are scoped to the owning editor/document.",
        "failureAndCancellation": "Unsupported partial structural cases reject before publication; allocation boundary maps named failures.",
        "testCoverage": "Support.Tests.RichSemanticEditor and Controls.Tests.RichSemantic cover source/selection/undo expectations. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextEditor.cpp",
      "sha256": "8aeb1af2fd4a675acdde047085f0b4a049e7f2ae819efdc04035f8262f8156c2",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Editor commands route into RichTextCommands and then publish document/history changes.",
        "ownershipAndLifetime": "Editor state owns selection, undo stack and document revision; callbacks stay UI-thread scoped.",
        "failureAndCancellation": "Failed transactions preserve prior selection/source and do not emit success notifications.",
        "testCoverage": "Controls.Tests.RichTextEditor and RichSemanticEditor cover key paths. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.RichTextLayout.cpp",
      "sha256": "886be4434a3b72dc8c3338fb6be08e6a8589bc76261bb1534a55fb72f0ffac51",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout",
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in this file during this read.",
      "reviewNotes": {
        "callers": "PreparedLayout serves PreparedDocumentLayout and PreparedSemanticGeometry for DirectWrite geometry, painting and hit tests.",
        "ownershipAndLifetime": "Layout stages COM layout, inline objects, clusters, lines and metrics; source/display mappings remain public coordinates.",
        "failureAndCancellation": "Input/range checks precede publication; transient shared-layout drawing state restored by scope_exit.",
        "testCoverage": "Controls.Tests.RichTextLayout and semantic geometry/raster fixtures; not run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.SemanticExtensions.h",
      "sha256": "1c44749aece263be944fb61519c8b1f9fe2928b19bf272d149a11402bee2b1e8",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticDiagrams"
      ],
      "findingIds": [],
      "disposition": "Full-file read was completed in the retained review history; no confirmed defect found in the current bounded review.",
      "reviewNotes": {
        "callers": "Extension dispatch is family-scoped and returns typed native object variants.",
        "ownershipAndLifetime": "Prepared objects own derived model/layout and are immutable after publication.",
        "failureAndCancellation": "Unknown or invalid fences fail closed so document builder can retain exact source.",
        "testCoverage": "Support.Tests.SemanticDiagrams, Support.Tests.NativeFlowchartCorpus and Support.Tests.MathCorpus. Test coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "src/Controls/DxUi.TextElementsData.h",
      "sha256": "634788ce7e2fae26aea98501c6ad932a65dac6b9474bf8aa599434d51ca5af70",
      "category": "production",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Unicode scalar classification is consumed by RichText grapheme navigation/deletion helpers, which binary-search the generated ranges.",
        "ownershipAndLifetime": "Generated immutable constexpr data has pinned Unicode 17.0.0 version and generator/license provenance; no runtime ownership.",
        "failureAndCancellation": "Not applicable: static generated table; malformed UTF-16 handling is in the consumer helpers.",
        "testCoverage": "Controls.Tests.RichTextLayout and Controls.Tests.RichDocumentLayout exercise grapheme boundaries, combining sequences and supplementary scalars; no tests run here."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.MarkdownCorpus.h",
      "sha256": "d617f056df524bfc6eb76bbe990c7c83a5b594143e730c5d39f940f27861fc02",
      "category": "tests",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Generated static fixture reviewed via format, provenance, consumer/oracle integration and complete case manifest. The Markdown header has an explicit entry-by-entry manual interpretation gap noted in testCoverage; no confirmed runtime defect.",
      "reviewNotes": {
        "callers": "This is generated data only; Controls.Tests.RichMarkdown and Controls.Tests.RichSemanticDocument iterate the CommonMark/GFM source, block expectations and entity table. Generator inputs are SHA-pinned; its full corpus counts are 652 CommonMark, 672 GFM and 2,125 entity values.",
        "ownershipAndLifetime": "The header consists of constexpr string views/records with no runtime ownership. The generator emits UTF-16 escapes to preserve exact source units.",
        "failureAndCancellation": "Source-input integrity is pinned and the generator rejects incomplete corpus counts. Entry-level output semantics are checked by the downstream corpus test suite; I did not execute it.",
        "testCoverage": "Controls.Tests.RichMarkdown (RunRichMarkdownTests, corpus and entity cases), Controls.Tests.RichSemanticDocument; Tools/Generate-MarkdownTestData.ps1 and Test-MarkdownData.ps1 own deterministic generation/check. Review limitation: I checked section boundaries, consumers, source pins/count invariants and generated representation; I did not manually interpret all 2,125 raw entries."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.MarkdownCorpus.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.PreparedTreeAccessibilityEvents.h",
      "sha256": "dabba04d347eb9799999a5f45d70a6227c76550597f4626189cab2eca105e52f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The harness runs a real UIA client on an MTA jthread and exercises callbacks through the native ControlHost provider.",
        "ownershipAndLifetime": "COM/event objects use WIL smart pointers and events; callback entry uses mutex-protected admission counts and teardown unsubscribes then drains accepted callbacks.",
        "failureAndCancellation": "Timeout gates bound reentrant notifications and teardown; root replacement and burst replacement assert retained source generations retire.",
        "testCoverage": "TestPreparedTreeSourceReplacementNotifiesReentrantNativeClient, TestPreparedTreeSourceBurstRetiresIntermediateSourcesDuringNativeCallback, TestPreparedTreeInvalidCountFallsBackToCurrentModel, TestPreparedTreeCaptureCanRetireItsControl."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.PreparedTreeAccessibilityEvents.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocument.cpp",
      "sha256": "427e475aeb8d6deca7436fbe29acab885d78713e019cc6f15519acb0dedc5b44",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite exercises Document and SourceSnapshot value APIs plus source replacement helper.",
        "ownershipAndLifetime": "Snapshots are held across edits/resets and concurrency test captures immutable values.",
        "failureAndCancellation": "Rejections use named standard exceptions and assert unchanged source/revision/history.",
        "testCoverage": "Tests cover Unicode/CRLF bounds, UTF-8, randomized edit oracles, history, persistent split/merge, maximum fragmentation and borrowed input. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocument.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocument.h",
      "sha256": "06bc81affee80252523e07198cf1c4b1ae4b7a9d84c6e9b5b8d13957bce7075a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichDocument.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocument.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocumentLayout.cpp",
      "sha256": "6773e332e2db89769de31e777d453938674451b5e6365dc224e0066255d7038f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "PreparedDocumentLayout is compared against flat PreparedLayout and raw DirectWrite observations; wrapper callers also compare actual ControlHost rendering.",
        "ownershipAndLifetime": "WIL COM wrappers own DWrite/D2D interfaces, while test-created spans and vectors own probe/raster buffers for call duration.",
        "failureAndCancellation": "Independent geometry, cancellation, resource-loss, range/cap and atomic-publication cases cover the retained chunk pipeline.",
        "testCoverage": "TestRichDocumentLayoutWholeGeometryOracle, TestRichDocumentLayoutWarpRasterOracle (108 exact WARP image pairs), TestRichDocumentLayoutCooperativeBudgetsAndGeometry, TestRichDocumentLayoutCoalescingCancellationAndFailure."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocumentLayout.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichDocumentLayout.h",
      "sha256": "2a2518e1d6f1486081d92b9c7c65585e9ae35b012f2291d25fb6de1488024aee",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "RunRichDocumentLayoutTests is registered by the Controls test runner and implemented in the paired .cpp file.",
        "ownershipAndLifetime": "Declaration-only header; no owned state.",
        "failureAndCancellation": "No independent failure path at the declaration boundary.",
        "testCoverage": "Exact test names and coverage are recorded on Controls.Tests.RichDocumentLayout.cpp."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichDocumentLayout.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichMarkdown.cpp",
      "sha256": "71e5ebdb70750020ba821e96a687763e72c3d2d19edbdd0f497403b31b1d78aa",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite routes to independent block/inline/capacity/HTML/footnote tests and native flowchart fixtures.",
        "ownershipAndLifetime": "Published syntax and parser snapshots are retained immutably across cancellation, revision and profile changes.",
        "failureAndCancellation": "Every parsing scenario checks exact partition/source bounds and unsupported inputs remain literal or receive bounded diagnostics.",
        "testCoverage": "Tests cover full CommonMark/GFM corpora, semantic expectations, extension identity, nesting budgets, native flowchart model/capacity and footnotes. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichMarkdown.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichMarkdown.h",
      "sha256": "c57c2df2a3dbae9068f1c75989ededb0417727d6354dc93df361682f9ef147bd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichMarkdown"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichMarkdown.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichMarkdown.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichProjectionBlocks.cpp",
      "sha256": "6a5448a7d3a1eb940e39ed94f5a38a7c6ccbc8e8e3f6876752141de31ea0be3b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichProjectionBlocks"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "PreparedProjection is compared against full Project output across source and rich modes.",
        "ownershipAndLifetime": "Held immutable blocks and clones are checked after edits, clear, moves, and sibling revisions.",
        "failureAndCancellation": "Invalid projections/ranges reject and preserve prior publication; block overflow rebuilds or falls back coherently.",
        "testCoverage": "Coverage includes complete ACP mapping, source ranges/styles, line joins, Unicode edits, capacity and fixed descriptive trials. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichProjectionBlocks.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichProjectionBlocks.h",
      "sha256": "aff2d33a58e4ee190f725d0a45aa74f9c191757f91cf839e211db7d7ab1a0fa3",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichProjectionBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichProjectionBlocks.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichProjectionBlocks.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemantic.cpp",
      "sha256": "8f499fa3c17c34dce130a1d81807d9aa9ec09a0c9709873aa7b9285e3d7ce256",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Native semantic entrypoints invoke focused parser, layout and editor command suites.",
        "ownershipAndLifetime": "Test helpers own local prepared objects and release them at case boundaries.",
        "failureAndCancellation": "Failure paths assert stable source/output expectations rather than silently skipping.",
        "testCoverage": "Relevant suites include native structural commands, family grammar and independent layout goldens. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemantic.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemantic.h",
      "sha256": "20b62e004bb09d02a3a055f235922dced1c14e519236dcca0ce7370b9fb05d3e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemantic.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemantic.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticDocument.cpp",
      "sha256": "7829b897c99b27b26dd455f1ff9fe1c63c7c4fc7cfbac06d4de6cb1cdf7b2e34",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite calls parser, semantic builder, native math/diagram prepare paths and semantic fixtures.",
        "ownershipAndLifetime": "Model expectations are value-based; parser publications and retained documents remain independently held.",
        "failureAndCancellation": "Rejected syntax checks exact source fallback and owned diagnostics; native test helper budgets are bounded where explicit.",
        "testCoverage": "Coverage includes math parser/layout, Mermaid grammar, semantic corpus, front matter/footnotes, source ownership, raster and source-region cases. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticDocument.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticDocument.h",
      "sha256": "0132c82758ed7a5ba3753936e08cc62c693e73e4d4bd77ccba2af5a675ea6b72",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticDocument"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemanticDocument.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticDocument.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticGeometry.cpp",
      "sha256": "0ecfd9f38105257c102a876be4fe008aeedacbceaa079fe5eb8c9b018dcd883b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite composes semantic geometry/editor fixtures through native and embedded host helpers.",
        "ownershipAndLifetime": "Host/editor fixtures are scoped around each scenario and prepared objects are released with their owners.",
        "failureAndCancellation": "Fallback ownership test rejects arbitrary/partial source intervals while retaining the prior publication; rendering-admission failure has explicit suite coverage.",
        "testCoverage": "Named tests cover geometry, carets, links/HTML, task markers, footnotes, Mermaid objects and failure containment. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticGeometry.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSemanticGeometry.h",
      "sha256": "e9aadf9681b4914d8777c9a95e22247e3fc684aae2f27742b100cfbaf241f635",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSemanticGeometry.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSemanticGeometry.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSourceChanges.cpp",
      "sha256": "75573cb9d41cf6dc30349b7a835ee8b4c6dc3ea79bf3dd44236770f0032e3a2a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "SourceSnapshot::ChangedRangeFrom is exercised after Document edits, undo/redo, transient previews and branch copies.",
        "ownershipAndLifetime": "Snapshots and expected flat strings are value-owned through concurrent reads.",
        "failureAndCancellation": "Every interval is checked for bounded scalar/CRLF edges and exact prefix/replacement/suffix reconstruction.",
        "testCoverage": "Tests cover recent-window tracking, expiry to full replacement, reset, random Unicode, moved/default snapshots. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSourceChanges.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichSourceChanges.h",
      "sha256": "dd68969d800689b9e06ac859dade916d76eebe393ef1a58e851671767830217f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichSourceChanges"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichSourceChanges.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichSourceChanges.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextAccessibility.cpp",
      "sha256": "5afdd72e02c955a910af4e6b930f7fb1d90eafb3840b532db4e030bf4e189e36",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Window and embedded providers share scenario helpers with explicit backend parameter.",
        "ownershipAndLifetime": "UIA provider references are reacquired around document revisions and range operations.",
        "failureAndCancellation": "Failure cases exercise native/literal Math fallback without changing source range ownership.",
        "testCoverage": "Coverage includes provider type/name/help text and exact source ranges; parent combined-14 reports EditorControls/Embedded pass. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextAccessibility.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextAccessibility.h",
      "sha256": "51e75ae7a64a18386718b4a69c56fac5e8713b299a2cb92d34e4b9c8aad82c1d",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextAccessibility.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextAccessibility.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextEditor.cpp",
      "sha256": "09937127179152c141dec628daab70ad3413190de618c5af924bb4066d71525f",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Tests exercise RichTextEditor through ControlHost/native messages, text input service imports, clipboard services and Markdown semantic preparation.",
        "ownershipAndLifetime": "Control root owns editors; COM/resource/request state uses WIL/shared ownership, and callbacks deliberately cover reentrancy, replacement, root retirement and host rebinding.",
        "failureAndCancellation": "Coverage includes source/selection/history atomicity, transient TSF preview/commit, bounded cooperative preparation, stale completion, fallback, cancellation and remote-image opt-in.",
        "testCoverage": "TestRichTextEditorCommittedTextThenSelectionCallbacks, TestRichTextEditorNativeCooperativePreparation, TestRichTextEditorNativeFlowchartRenderingAndFallbacks, TestRichTextEditorRemoteImagePolicy; this agent did not execute tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextEditor.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextEditor.h",
      "sha256": "f7eba1c7d65e589e81a146d1c8312f7e1548c0e695835fa019d0d511cf70dd64",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextEditor.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextEditor.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextInput.cpp",
      "sha256": "9291a288f02b17bd9c5d5c8105a45eaf16f55405b724a3908932af5a20a3ab60",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextInput"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Exercises production TextInputServices, EmbeddedHost, RichTextEditor and application RichImageLoader paths.",
        "ownershipAndLifetime": "WIL handles, scope guards, service/client objects and request values are scoped through completion/teardown.",
        "failureAndCancellation": "Stale revision, read-only, active composition, cancellation, queue overflow, rebinding and disconnected-provider paths assert fail-closed behavior.",
        "testCoverage": "Suite includes clipboard/image source transactions, IMM preview/result, mapped geometry, composition, and image loader lifecycle. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextInput.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextInput.h",
      "sha256": "16c8bb007e1af52583f3203e3fe3b02830e77ae41c849c42fbc427d6c9924bb9",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextInput"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextInput.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextInput.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextLayout.cpp",
      "sha256": "c4387b284e2cc2efea7c68b2227cf34ce11bd20694639ddee5f6372e980169df",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Suite drives PreparedLayout, Unicode navigation, source-ink preparation and prepared flowchart raster helpers.",
        "ownershipAndLifetime": "DirectWrite/Direct2D objects use WIL owners; prepared copies are tested against shared mutable drawing effects.",
        "failureAndCancellation": "Malformed projections/options reject while prior geometry remains coherent; capacity paths keep source ink unpublished.",
        "testCoverage": "Tests cover DPI-independent geometry, Unicode grapheme stops, typography, link/source ink, borrowed targets and flowchart raster/budget. Test attribution only; this agent did not run it."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextLayout.cpp"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Controls/Controls.Tests.RichTextLayout.h",
      "sha256": "193d05a45396851b88b45a0b77124d7af53130d3f02641901e11a2046f0b42ce",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Controls.Tests.RichTextLayout"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; this translation unit only declares its test-suite entrypoint and introduces no independent behavior.",
      "reviewNotes": {
        "callers": "The corresponding .cpp file defines the suite; the project test runner calls this named entrypoint.",
        "ownershipAndLifetime": "Header declares no state, resources, or ownership.",
        "failureAndCancellation": "No failure or cancellation behavior is implemented in this declaration-only header.",
        "testCoverage": "Suite implementation is identified by Controls.Tests.RichTextLayout.cpp; this agent did not run tests."
      },
      "plannedPath": "Tests/Controls/Scope.Tests.RichTextLayout.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityBenchmark.h",
      "sha256": "ad9d87bd9677c7d6b41dae0f21a7eedd1406ab17c5e4d552298f4fac08d9c4d0",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityBenchmark"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "The benchmark prepares identical source and controls before timing snapshot updates.",
        "ownershipAndLifetime": "Providers and fixture model stay alive through the measured and post-measurement accessibility checks.",
        "failureAndCancellation": "Detach explicitly tests stale provider behavior after measured phases.",
        "testCoverage": "Outputs descriptive count/allocation/timing samples without imposing unstable timing thresholds. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityFixture.h",
      "sha256": "0494cf3af7ed90d4b52816e2983c565f952ac2a6c063de7c53db596579f7a8e7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityFixture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Synthetic 10,000-row model defines stable literal IDs and labels independently.",
        "ownershipAndLifetime": "Model owns row names and exposes only views during provider access.",
        "failureAndCancellation": "Out-of-range and missing IDs return empty results; counters are owner-thread scoped.",
        "testCoverage": "Consumed by large prepared-row fixture and semantic tests. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilityRowsFixture.h",
      "sha256": "016746cfa75fcdd3cbf1513ba7b0536caf47ac10cf70445df2490992102d8715",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibilityRowsFixture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Prepared row adapter copies fixture names before publication and exposes immutable views.",
        "ownershipAndLifetime": "Worker construction joins before prepared rows are exposed to controls/providers.",
        "failureAndCancellation": "Feature-guarded adapter avoids requiring unsupported public API in baseline builds.",
        "testCoverage": "Consumed by large tree accessibility benchmark. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeAccessibilitySemantics.h",
      "sha256": "78ec16b0f2f641f03ac0d3129d2beb7d0cbb3fb99894ecb74342dd01204f9289",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The embedded accessibility provider consumes immutable prepared Tree row snapshots attached to EmbeddedHost.",
        "ownershipAndLifetime": "Providers retain snapshot owners until root replacement/detach; test uses shared witness counters to verify snapshot lifetime exactly.",
        "failureAndCancellation": "Same-count same-id content replacement updates retained provider names, then replacing root/detaching returns ELEMENTNOTAVAILABLE safely.",
        "testCoverage": "TestPreparedTreeAccessibilitySemantics; comment explicitly excludes reentrant-client, large-count and G1 claims."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeEvents.h",
      "sha256": "e9066ca1dbeebe69d8bcc81467af2948302bcbbe46d8939074322dc9c47063ff",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "EmbeddedHost UIA bridge publishes property and structure events from prepared snapshots to real MTA client callbacks.",
        "ownershipAndLifetime": "Observer callback admission is mutex-protected and teardown closes admission, unsubscribes, drains accepted callbacks, then releases retained UIA elements and labels.",
        "failureAndCancellation": "Gated notification bursts assert intermediate snapshots release, latest property is coalesced, labels remain valid only while alive, and root replacement invalidates elements.",
        "testCoverage": "TestPreparedTreeSourceReplacementNotifiesReentrantEmbeddedClient, TestPreparedTreeSourceBurstRetiresIntermediateSourcesDuringEmbeddedCallback, TestPreparedTreePropertyBurstCoalescesWithoutRetainingSources, TestPreparedTreeLabeledByBurstCoalescesWithoutRetainingSources."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.PreparedTreeRootLifetime.h",
      "sha256": "8dff9321137b6c61536ad7b03f5e000f2f41a6c43c8cc1c568d61f9db69332f4",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeRootLifetime"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Root-lifetime test replaces source while retaining root and old providers.",
        "ownershipAndLifetime": "Borrowed fixture root is cleared before teardown; COM ownership uses WIL holders.",
        "failureAndCancellation": "Checks detach invalidates retained provider and superseded source is released.",
        "testCoverage": "TestPreparedTreeRootDoesNotRetainSupersededSource. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichDocumentStorageResources.h",
      "sha256": "acc5e0701101b22711ced325c2b26cc862d7d56dab1b6ba53a1f924b9230f686",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Allocation/retained-byte instrumentation wraps Document source replacement, undo/redo, transient metadata and source-change snapshots.",
        "ownershipAndLifetime": "The fixtures compare tracked live bytes and explicitly verify release to zero after scoped document/history destruction.",
        "failureAndCancellation": "Injected bad_alloc points assert source, selection, revision and full undo/redo history remain coherent; transient metadata and equality queries have bounded/no allocation checks.",
        "testCoverage": "TestRichDocumentFragmentedHistoryStorageBudget, TestRichRecordedWholeTextStorageBudget, TestRichDocumentAllocationFailureAtomicity, TestRichSourceChangeRolloverFailureAtomicity."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichDocumentStorageResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichGeometryResources.h",
      "sha256": "e47e11ce969bca72996df2c3d94eb69f079eb33ee2d8c39cb3ef801fcc83df5e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "The fixture drives PreparedLayout and PreparedDocumentLayout via supplied DirectWrite/WARP host APIs.",
        "ownershipAndLifetime": "Document snapshots and immutable projection values are owned; WIL owns COM factories; counters are scoped and restored.",
        "failureAndCancellation": "Injected allocation points preserve prior source/projection/geometry, retries succeed, max retained bytes and retirement are bounded; composition checks no allocations.",
        "testCoverage": "TestRichPreparedLayoutClearAllocationBudget, TestRichPreparedLayoutValueFailureAtomicity, TestRichDocumentGeometryFailureAtomicity, TestRichDocumentGeometryMaximumStorageBudget, TestRichDocumentGeometryReplacementPeakBudget."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichGeometryResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichImageCapture.h",
      "sha256": "5fc76d8d2f5dbe6cde884f4af54ca7ac66d3b61aaffa1d3d7a0a097e3b19100a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.RichImageCapture"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no confirmed defect found in this fixture/test helper.",
      "reviewNotes": {
        "callers": "Harness capture uses deterministic invented paths and generated pixel fixtures.",
        "ownershipAndLifetime": "Image data is owned by RichImage and request objects remain scoped through completion.",
        "failureAndCancellation": "Missing-file path is explicitly retained as fallback and source/history are asserted unchanged.",
        "testCoverage": "CaptureRichImageSample exercises the actual embedded image sample. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichImageCapture.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichImageFailures.h",
      "sha256": "0d1802fcdd7e32e8af39f124b6c369bded4518a15ee4369a6aa2eef7b856bf84",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "RichImageResources and prepared semantic geometry are exercised through bounded staged image admission and owner completion APIs.",
        "ownershipAndLifetime": "Caller-held request tokens and immutable pixel buffers are value/shared-owned; resource clear/control teardown cancels tokens.",
        "failureAndCancellation": "Allocation/decode/driver failures retain alternatives, preserve prior publication, honor limits and do not create idle retry loops.",
        "testCoverage": "Image resource allocation-failure and raster/driver failure cases in Embedded.Tests.RichImageFailures; reviewed source only, no test run."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichImageFailures.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichProjectionResources.h",
      "sha256": "e3406c1d831e61d630326a0e4d434ecaa702cb4155e18081ca8469db3ddbf034",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Projection snapshot slicing and PreparedProjection consume immutable Document snapshots and source-change metadata.",
        "ownershipAndLifetime": "Expected and actual projections own source, maps, units and runs; counters are restored through scoped guards.",
        "failureAndCancellation": "Allocation sweeps cover slicing, preparation and flat projection while preserving prior published values; mapping queries and retirement are checked allocation-free.",
        "testCoverage": "TestRichProjectionSnapshotAllocationFailureAtomicity, TestRichPreparedProjectionFailureAtomicity, TestRichFlatProjectionFailureAtomicity."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichProjectionResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticDocumentResources.h",
      "sha256": "174d82358b617ffc2e06ea344ccbf7486094cb49ac5c66f4e37a5329ee9c38e5",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "SemanticBuilder consumes published MarkdownSyntax and publishes immutable SemanticDocument values.",
        "ownershipAndLifetime": "Old/current syntax and semantic values are shared owners; failure fixtures retain old publication and inspect tracked live bytes.",
        "failureAndCancellation": "Allocation sweeps cover complete build and Begin admission; explicit retry, unchanged request coalescing and max-source retained-byte cap are asserted.",
        "testCoverage": "TestRichSemanticDocumentFailureAndStorageBudgets."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticDocumentResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticGeometryResources.h",
      "sha256": "14663ee4bdf2be80cbf4649cf051e32d5a360464634c37cfc90b879d278433de",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "PreparedSemanticGeometry is prepared from SemanticDocument plus layout options, while editor text-service queries consume its immutable geometry.",
        "ownershipAndLifetime": "Shared semantic/document inputs and WIL DirectWrite factory are retained during preparation; resource graph is explicitly cleared after allocation tracking.",
        "failureAndCancellation": "Query allocation failures return unavailable without source/history/geometry mutation; Begin allocation sweeps preserve prior publication and allow retry.",
        "testCoverage": "TestRichSemanticEditorQueryFailureBudgets, TestRichSemanticGeometryFailureBudgets (including native spacing and footnote modes)."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticGeometryResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichSemanticResources.h",
      "sha256": "ec2b7a3004301404fce848f78300a8a82785d84ab43b344de8892542e81c0af2",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Exercises semantic coordinate maps, Markdown parser/builder, semantic geometry and flowchart native preparation, plus real native/embedded UIA dispatch.",
        "ownershipAndLifetime": "Expected source/map values are separately constructed; immutable semantic documents and prepared resources retain shared ownership; UIA clients subscribe/unsubscribe and callback gates protect lifetime.",
        "failureAndCancellation": "Fault sweeps cover parser/builder/geometry/flowchart allocation failures, retry and pending preservation; UIA collection and owner-dispatch failures return HRESULTs; max map bytes and scalar no-allocation queries checked.",
        "testCoverage": "TestRichSemanticNativeDispatchFailureAtomicity, TestRichSemanticAccessibilityFailureAtomicity, TestRichMarkdownCommandHostFailureAtomicity, TestRichMarkdownCommandFailureAtomicity, TestRichSemanticFailureAndStorageBudgets."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichSemanticResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Embedded/Embedded.Tests.RichTextResources.h",
      "sha256": "b0396960cb3125aa7c13342c6fb532ca0dc97ea83e6635bf715aa110375b78a7",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the examined boundary. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Broad supplied-device fixture integrates Markdown/parser, flowchart, semantic, rich-image, grapheme, text-input and retained editor behavior through EmbeddedHost.",
        "ownershipAndLifetime": "GraphicsDevice pools are shared owners, EmbeddedHost owns editor/control state, immutable syntax/semantic/image values retain explicit owners, and UIA callbacks use scoped detach/drain paths.",
        "failureAndCancellation": "Covers staged preparation failure/cancellation, source fallback and retention, semantic allocation sweeps, images, source/history, identity, pending input, no-activation shared views, Unicode capacity, UIA metadata limits and retry.",
        "testCoverage": "TestNativeFlowchartAllocationFailureAtomicity, TestRichMarkdownParserFailureAndStorageBudgets, TestEmbeddedFlowchartRenderingAndFallbacks, TestRichTextCooperativeSchedulingAndTextInput, TestRetainedRichTextUiaRangeAllocationBudget, TestUnicodeGraphemeForwardWalkAllocationBudget. Test code read only; no execution by this agent."
      },
      "plannedPath": "Tests/Embedded/Scope.Tests.RichTextResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.FlowchartLayout.h",
      "sha256": "df668ea737758094caa33856784e07275552847db9e1d3c8a53f84d55914f38a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.FlowchartLayout"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Corpus covers admitted source-reference graphs.",
        "ownershipAndLifetime": "Geometry pins rank, routes and typed relations.",
        "failureAndCancellation": "Fanout checks finite geometry, endpoint ports and resource bounds.",
        "testCoverage": "Invariants checks extents and endpoint boundary constraints. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedPath": "Tests/Support/Scope.Tests.FlowchartLayout.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownBlocks.h",
      "sha256": "b48e039f996317a7db4fbb45951cd39ea3d917c53385a713533ce564f684c727",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MarkdownBlocks"
      ],
      "findingIds": [],
      "disposition": "Full-file review completed; parent-reported focused diagnostics passed in combined-14. Broader six-profile qualification remains pending.",
      "reviewNotes": {
        "callers": "CheckOriginalCorpus checks all 105 original outcomes, with case indices on mismatches.",
        "ownershipAndLifetime": "CheckFamilyGrammar uses independently authored normalized model and diagnostic expectations.",
        "failureAndCancellation": "CheckFanout checks stable Cartesian order, source ranges, caps and original cases 80/81.",
        "testCoverage": "Parent evidence: .build/regression-20261024-red/combined-14/state-description-after.log and results.json show state-description-after, flowchart-allocation, full EditorControls, and Embedded exitCode 0. This agent did not run tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownBlocks.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownCapacity.h",
      "sha256": "5ce29010056b90d2a55c69440244017a3afd59dc3aa0adedf56b00bd4506c88e",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownCapacity.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownCapacity.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownCapacity.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownHtml.h",
      "sha256": "a4d40be77823bbc132c571247cd85b49bbca369281f10dae4d7b102349149bac",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownHtml.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownHtml.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownHtml.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownInlines.h",
      "sha256": "9299d4177a333d3be31666b0ee12121dd0abbc27c483ce5b5ee16b91a3dce7db",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Shared fixture scenarios are consumed by Markdown parser/model tests and editor-level profile tests.",
        "ownershipAndLifetime": "Static expected strings and source snippets are constexpr views owned by the test binary.",
        "failureAndCancellation": "Cases compare independently authored inline syntax/output for profiles; no mutation or asynchronous state in this fixture.",
        "testCoverage": "Support.Tests.MarkdownInlines; Controls.Tests.RichMarkdown and Controls.Tests.RichTextEditor reference these scenarios."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownInlines.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MarkdownSourceCorrelation.h",
      "sha256": "3fa85960dae726967a49772bbfe06f9e883647c9b2839619d8b3165eed324cef",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.MarkdownSourceCorrelation.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.MarkdownSourceCorrelation.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.MarkdownSourceCorrelation.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.NativeFlowchartCorpus.h",
      "sha256": "bfe2b2fb4f37cc72e4e4dd94c853ecdd3b533f573c013ed30f01a5d8ae12e9c3",
      "category": "tests",
      "generated": true,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Generated static fixture reviewed via format, provenance, consumer/oracle integration and complete case manifest. The Markdown header has an explicit entry-by-entry manual interpretation gap noted in testCoverage; no confirmed runtime defect.",
      "reviewNotes": {
        "callers": "Generated pinned Mermaid 11.4.1 fixture data feeds Support.Tests.MarkdownBlocks, Support.Tests.FlowchartLayout, Controls.Tests.RichMarkdown, and Embedded.Tests.RichTextResources.",
        "ownershipAndLifetime": "Fixture entries are static constexpr UTF-16 views and expected outcome/count/direction records; source documentation bytes remain separately pinned and generator-owned.",
        "failureAndCancellation": "Decoded a concise manifest of all 105 numbered original cases and enumerated 20 family rows: 16 prepared original cases, 89 unsupported original cases, 1 prepared state family case, and unsupported non-flowchart families with typed reasons. Fan-out originals 80/81 expect four edges. No tests executed by this agent.",
        "testCoverage": "Support.Tests.MarkdownBlocks (TestNativeFlowchartPinnedIndependentCorpus), Support.Tests.FlowchartLayout, Controls.Tests.RichMarkdown, Embedded.Tests.RichTextResources. Parent-run evidence read from .build/regression-20261024-red/combined-14/results.json and state-description-after.log: targeted original-corpus test, flowchart-allocation sweep, full EditorControls, and Embedded all report exitCode 0."
      },
      "plannedPath": "Tests/Support/Scope.Tests.NativeFlowchartCorpus.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PreparedFlowchart.h",
      "sha256": "2f95dbf492fe773a9859a1df7a379a3f2eeeea17590a763117031f712c750e85",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.PreparedFlowchart"
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed; no confirmed defect found in the bounded review. Historical Mermaid-specific dispositions are in MermaidRevalidation_2026-10-24.md; native test results await parent qualification.",
      "reviewNotes": {
        "callers": "Coalescing covers same-instance intrinsic option reuse.",
        "ownershipAndLifetime": "Relations checks hollow inheritance and distinct association raster output.",
        "failureAndCancellation": "StadiumRaster checks curved capsule output.",
        "testCoverage": "Labels checks native measured label geometry, bidi and multiline behavior. Tests referenced as coverage only; this agent did not run them."
      },
      "plannedPath": "Tests/Support/Scope.Tests.PreparedFlowchart.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImage.h",
      "sha256": "1535e9a96e6a57675f9f3119305ae6f7e98385bde75f09b6106676973df3b171",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImage"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Image parser/model tests use independent expected source and dimensions.",
        "ownershipAndLifetime": "Fixture buffers own decoded image bytes for the duration of the case.",
        "failureAndCancellation": "Unsupported and malformed image input assert rejection without partial model.",
        "testCoverage": "Support.Tests.RichImage covers metadata and source normalization. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImage.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImagePaths.h",
      "sha256": "ba1db2f67609d4fdce5d1a6b0ee79d49138eb661faa3b8beaeb42611ffc70634",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichImagePaths"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Path cases independently cover local paths, URI forms and traversal policy.",
        "ownershipAndLifetime": "Resolver results are value-owned paths with bounded lengths.",
        "failureAndCancellation": "Invalid schemes, encodings and traversal cases are rejected.",
        "testCoverage": "Support.Tests.RichImagePaths covers path admission and canonicalization. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImagePaths.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichImageResources.h",
      "sha256": "6620340033708ffa3ea0ad56cf2cd36fef1f8c5562ebc417fed015bbdc3e1107",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [],
      "findingIds": [],
      "disposition": "Full current-file read completed; no new confirmed defect found. Coverage is source attribution only; this agent did not execute tests/builds.",
      "reviewNotes": {
        "callers": "Fixture constructs independent SemanticDocument values and exercises RichImageResources plus RichTextEditor image request APIs.",
        "ownershipAndLifetime": "Decoded RichImage payloads are immutable shared owners; request tokens are copied/moved to explicitly test cancellation/retirement.",
        "failureAndCancellation": "Covers admission ceilings, path deduplication, shared decoded-byte accounting, current/stale completions, source/base/mode/visibility changes, no retry loop and actual raster pixels.",
        "testCoverage": "RichImageResourceTests.Admission, ReadyPicture, Pixel, PatternPixels, Retained; called by Controls.Tests.RichTextEditor and embedded image tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichImageResources.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemantic.h",
      "sha256": "1fc507d8b00161724e5d94381a3c68bd0c78dbbb9fe68fdadf498941f86c2556",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemantic"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Parser contract helpers create independent expected node/diagnostic values.",
        "ownershipAndLifetime": "Semantic input owns source ranges and model values; no pointer-lifetime shortcut is used.",
        "failureAndCancellation": "Unsupported branches assert retained source and diagnostic outcome.",
        "testCoverage": "Structural command model cases include selection/source/history expectations. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemantic.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticDocument.h",
      "sha256": "aebe9e4b7b1fab8636f84a7842e2ddcfa658627798ffcb4956768251a9ef643c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Related tests for Support.Tests.RichSemanticDocument.h in the owning DxUi Support.Tests or Controls.Tests suite; exact case names are listed in the corresponding test row."
      ],
      "findingIds": [],
      "disposition": "Full current-file read completed during this review. No confirmed defect in the code boundary examined; tests/builds not run by this agent.",
      "reviewNotes": {
        "callers": "Full file read traced the public or production entry points for Support.Tests.RichSemanticDocument.h to its owning parser, document, layout or editor boundary.",
        "ownershipAndLifetime": "State is staged or value-owned at the component boundary; caller retains immutable publication or explicit owner-thread lifetime as defined in the local API.",
        "failureAndCancellation": "Admission/range checks precede publication where applicable; malformed input or preparation failures preserve source or prior committed state through the existing failure contract.",
        "testCoverage": "Reviewed related named fixtures in the owning Support.Tests / Controls.Tests partition; source oracles were inspected, not executed. No repository-wide test claim."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticDocument.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticEditor.h",
      "sha256": "73545f3be04226082577e40b0f7886745141b8fc8b76fb280aaa57e37b5f2c0b",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Editor command fixtures exercise full source-to-selection mutation paths.",
        "ownershipAndLifetime": "Test owners hold snapshots/history through undo and redo comparisons.",
        "failureAndCancellation": "Failure cases assert source and selection preservation after rejected commands.",
        "testCoverage": "Cases cover tables, lists, headings, quote/list Enter and inline-to-code conversions. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticEditor.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichSemanticGeometry.h",
      "sha256": "2e3b7ce22ec8fba909e81713ea386cfa1eb6087d2e7cab6b1cbf3e0b1a5e8992",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticGeometry"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Geometry expectations are independently normalized and finite.",
        "ownershipAndLifetime": "Fixtures own layout inputs and destroy prepared resources at case completion.",
        "failureAndCancellation": "Invalid geometry and resource limits check failure without partial publication.",
        "testCoverage": "Cases cover relation kind, endpoints, extents and routing bounds. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichSemanticGeometry.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.RichTextStateReplacement.h",
      "sha256": "ff74dde6002acb577b05d4b28f56657e81b7a81bf8b935fd1fde9bd32c0eda1a",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.RichSemanticEditor"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "State replacement fixture compares source before and after a scoped replacement.",
        "ownershipAndLifetime": "Selection and document revisions are captured as values.",
        "failureAndCancellation": "Rejected replacements preserve original content and history.",
        "testCoverage": "Tests target state replacement behavior independently of rendering. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.RichTextStateReplacement.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SemanticDiagrams.h",
      "sha256": "14f5bd5e0d6b39c9f5b38d860b5b23b09c009dfccab4229ec849c1e27ec0df2c",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticDiagrams"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Fixture scenarios distinguish accepted semantic fences from exact-source fallback.",
        "ownershipAndLifetime": "Normalized expectations include family, names and model relations.",
        "failureAndCancellation": "Fallback assertions check retained source and diagnostic classification.",
        "testCoverage": "Tests are consumed by semantic diagram and embedded resource suites. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.SemanticDiagrams.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.SemanticSourceBlock.h",
      "sha256": "cd2bb9f219d0fdbb222b6c2fc1288d759201e746d44d2a37c737a856f69bf492",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.SemanticSourceBlock"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Fence integration tests distinguish ready native objects from literal source fallback.",
        "ownershipAndLifetime": "Fixture asserts exact prefixes, payload and suffix across the source range.",
        "failureAndCancellation": "Failure expectations preserve source and verify diagnostic/retained state.",
        "testCoverage": "Semantic source block and fallback tests cover Markdown embedding boundaries. Coverage attribution only; this agent did not execute tests."
      },
      "plannedPath": "Tests/Support/Scope.Tests.SemanticSourceBlock.h"
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.PreparedTreeSemanticFixture.h",
      "sha256": "09704ea7fcbe7944fcf4e084d4c0678ba5e46e9284c099af50391e2cbeae61dd",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Embedded.Tests.PreparedTreeAccessibility"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed in review history; no confirmed defect found in this review.",
      "reviewNotes": {
        "callers": "Fixture constructs prepared semantic tree with explicit node ownership.",
        "ownershipAndLifetime": "Resource and document lifetime are scoped to test fixture teardown.",
        "failureAndCancellation": "Failure setup is isolated so partial preparation is observed safely.",
        "testCoverage": "Consumed by embedded semantic-tree accessibility event tests. Coverage attribution only; this agent did not execute tests."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tools/tests/Test-MarkdownData.ps1",
      "sha256": "8a65257dbb780d6849c3657a571e05b032acfc1ab116d3936fb7729d3cee48fe",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Test-MarkdownData.ps1"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; regeneration and stale/missing-pin coverage is explicit; no tooling defect confirmed.",
      "reviewNotes": {
        "callers": "Offline generator workflow invokes this script as a focused fixture test.",
        "ownershipAndLifetime": "Temporary inputs and outputs are created under the fixture root supplied by Invoke-FixtureCase.",
        "failureAndCancellation": "Test asserts missing/stale pins fail before output and preserves changed artifacts for diagnosis.",
        "testCoverage": "Covers deterministic output for four generated datasets, immutable check mode, and all declared data/license pins. This agent did not execute it."
      }
    },
    {
      "repository": "DxUi",
      "path": "Tests/Support/Support.Tests.MathCorpus.h",
      "sha256": "3e0be1acecdf04598563465c72144478f6bca29f04146caa64cbcc2998b4df90",
      "category": "tests",
      "generated": false,
      "reviewStatus": "reviewed",
      "reviewer": "mermaid_executor",
      "testIds": [
        "Support.Tests.MathCorpus"
      ],
      "findingIds": [],
      "disposition": "Full-file read completed; no additional defect confirmed in this review. Parent-reported diagnostic evidence is attributed separately.",
      "reviewNotes": {
        "callers": "Corpus cases define source, normalized expected math model and rejection reason independently.",
        "ownershipAndLifetime": "Expected node vectors are fixture-owned and compared after parsing, avoiding parser output as its own oracle.",
        "failureAndCancellation": "Cap and malformed cases retain exact source on rejection.",
        "testCoverage": "Support.Tests.MathCorpus and NativeMath test entrypoints. Coverage attribution only; this agent did not execute tests."
      },
      "addedAt": "execution-2026-10-24"
    }
  ]
}
```
