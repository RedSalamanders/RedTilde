#pragma once
#include "SpellProvider.h"
#include <DxUi/MarkdownSyntax.h>
#include <DxUi/RichDocument.h>
#include <DxUi/RichSemanticDocument.h>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace RedTilde
{
struct ProseChunk
{
    std::size_t baseOffset{};
    std::wstring text;
    bool operator==(const ProseChunk&) const = default;
};

struct ExtractedProse
{
    std::vector<ProseChunk> chunks;
    std::optional<std::wstring> frontMatterLanguage;
    bool semanticMode{};
};

// Script-aware uppercase filter: returns true only when cased uppercase letters are present (>= 2),
// zero lowercase letters are present, and no caseless-only scripts are misclassified as acronyms.
[[nodiscard]] bool IsScriptAwareAllUppercase(std::wstring_view word) noexcept;
[[nodiscard]] bool ContainsAsciiOrUnicodeDigit(std::wstring_view word) noexcept;
[[nodiscard]] bool LooksLikeInternetOrFilePath(std::wstring_view segment, std::size_t start, std::size_t length) noexcept;
[[nodiscard]] bool ShouldSkipSpellError(std::wstring_view segment, SpellErrorSpan span, const SpellCheckOptions& options) noexcept;

// Read-only front-matter language hint extractor (YAML/TOML top-level `lang:` / `language:` scalar, 1..32 ASCII [A-Za-z0-9-]).
[[nodiscard]] std::optional<std::wstring> ExtractFrontMatterLanguage(const DxUi::RichText::MarkdownSyntax* syntax, std::wstring_view sourceText);

// Compose / Split extractor: masks non-prose semantic spans 1:1 with U+FFFC so every display offset matches Display().
[[nodiscard]] ExtractedProse ExtractSemanticProse(const DxUi::RichText::SemanticDocument& document,
                                                  const DxUi::RichText::MarkdownSyntax* syntax,
                                                  std::wstring_view sourceText);

// Source-view extractor: masks non-prose Markdown blocks/inlines/destinations 1:1 with U+FFFC so every offset matches Source().
[[nodiscard]] ExtractedProse ExtractSourceProse(const DxUi::RichText::MarkdownSyntax& syntax, std::wstring_view sourceText);

// Map a provider-reported display range back to ascending, disjoint, UTF-16 boundary-valid raw source fragments.
[[nodiscard]] bool TryProjectDisplayErrorToSource(const DxUi::RichText::SemanticDocument& document,
                                                  DxUi::RichText::MarkdownRange displayRange,
                                                  std::vector<DxUi::RichText::MarkdownRange>& outSourceFragments);

// Plan one atomic undoable SourceEdit sequence replacing a multi-fragment or single-fragment word while keeping inline formatting intact.
[[nodiscard]] bool TryBuildSpellReplacementEdits(const DxUi::RichText::SourceSnapshot& source,
                                                 const std::vector<DxUi::RichText::MarkdownRange>& fragments,
                                                 std::wstring_view replacement,
                                                 std::vector<DxUi::RichText::SourceEdit>& outEdits);
} // namespace RedTilde
