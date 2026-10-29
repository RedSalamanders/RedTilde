# Specification authority

`AGENTS.md` and this index own repository policy. Domain contracts describe intended product behavior;
`capabilities.json` states current implementation and remaining gates. The exact dependency identity belongs
in `Dependencies/DxUi.lock.json`. An intended requirement is not evidence that the feature already works.

| Contract | Responsibility |
| --- | --- |
| [Architecture](Core/Core_Architecture.md) | Ownership, scheduling, limits and Red-family integration |
| [Editor](Editor/Editor_Markdown.md) | Lossless source, commands, views and resource content |
| [File safety](Core/Core_FileSafety.md) | Encoding, persistence, conflicts and recovery |
| [Settings and crashes](Core/Core_SettingsAndCrashes.md) | Live appearance, persistence and local diagnostics |
| [Design system](UI/UI_DesignSystem.md) | Writing experience, responsive layout and accessibility |
| [Build](Build/Build_Toolchain.md) | Pinned library and six build configurations |
| [Validation](Testing/Testing_Validation.md) | Scenario coverage, captures and qualification evidence |
| [Skill packages](Testing/Testing_SkillPackages.md) | Skill structure, package references and scoped-guidance inventory |
| [Performance](Testing/Testing_Performance.md) | Completed-frame pairs, provenance and native resource lifecycle |

Active sequencing is indexed in [Plans/WIP](Plans/WIP/README.md); completed records live in [Plans/Done](Plans/Done/README.md).

The [Changes workflow](Core/Core_Changes.md) owns fragment format, validation, and folding merged fragments on the default branch through a pull request of its own.

[RedMonitor integration](Core/Core_RedMonitorIntegration.md) owns the optional diagnostic client, SDK pin and viewer command.

Common native build policy, zero-warning acceptance, terminal output and short consumer paths: [Build_ConsoleAndPaths](Build/Build_ConsoleAndPaths.md).
