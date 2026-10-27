# RedTilde before/after tool metadata changes

This table groups every changed captured field by tool. changed-tool-settings.csv maps each change to project/configuration/platform and full before/after values. The comparison uses each project/profile field value set; full-tool-metadata.csv.gz retains exact per-file assignments. Literal unset/default differences and dependency-output fingerprints are preserved, not replaced with assumed compiler behavior. No native source files changed.

| Tool / field | Changed project-profile rows | Configurations | Before values | After values |
| --- | ---: | --- | --- | --- |
| ClCompile / AdditionalIncludeDirectories | 6 | ASan Debug, Debug, Release | project/file-specific; see CSV | project/file-specific; see CSV |
| ClCompile / ForcedIncludeFiles | 2 | ASan Debug | Z:\src\RedTilde\.build\dependencies\DxUi\source\dd619ef0c5855307eb57758072658f2e3b777fd8\src/Support/RequireAddressSanitizer.h; | &lt;REPO&gt;\.build\dependencies\DxUi\source\dd619ef0c5855307eb57758072658f2e3b777fd8\src/Support/RequireAddressSanitizer.h; |
| ClCompile / FunctionLevelLinking | 2 | Release | &lt;unset&gt; | true |
| ClCompile / WholeProgramOptimization | 2 | Release | false | true |
| Lib / LinkTimeCodeGeneration | 2 | Release | &lt;unset&gt; | true |
| Link / LinkTimeCodeGeneration | 2 | Release | &lt;unset&gt; | UseLinkTimeCodeGeneration |
| Link / OptimizeReferences | 2 | Release | &lt;unset&gt; | true |
