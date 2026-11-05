#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace RedTilde
{
struct Heading
{
    unsigned level{};
    std::size_t offset{};
    std::wstring text;
};
enum class DocumentNewlineKind
{
    None,
    Lf,
    CrLf,
    Cr,
    Mixed
};
struct DocumentLine
{
    std::size_t offset{}, length{}; // Source UTF-16 content, excluding CR/LF.
    bool operator==(const DocumentLine &) const noexcept = default;
};
struct DocumentInfo
{
    std::size_t words{}, characters{}, lines{};
    std::vector<Heading> headings;
    // Facts belong to the same source revision as characters/headings. Empty
    // source and a final newline each retain their real empty physical line.
    std::vector<DocumentLine> lineIndex;
    DocumentNewlineKind newlineKind = DocumentNewlineKind::None;
};
struct DocumentLineIndexStorageStats
{
    std::size_t reuseCount{}, capacityGrowthCount{}, capacityNormalizationCount{}, droppedSpareCount{};
    std::size_t activeCapacity{}, spareCapacity{};
};
// Keeps only a bounded retired line-index buffer between source revisions.
// This is application-internal storage policy, not a source-analysis cache.
class DocumentLineIndexStorage final
{
  public:
    [[nodiscard]] DocumentInfo Analyze(std::wstring_view text, std::size_t previousPhysicalLineCount = 0);
    void Retire(std::vector<DocumentLine> &&previous, std::size_t activeLineCount) noexcept;
    [[nodiscard]] const DocumentLineIndexStorageStats &Stats() const noexcept
    {
        return stats;
    }

  private:
    std::vector<DocumentLine> spare;
    DocumentLineIndexStorageStats stats;
};
struct DocumentPosition
{
    std::size_t line = 1, column = 1; // One-based source UTF-16 positions.
    std::size_t lineOffset{}, lineCharacters{};
    bool operator==(const DocumentPosition &) const noexcept = default;
};
// A previous revision's physical line count is only an allocation hint, bounded
// by the incoming source. It never supplies or preserves any metadata/storage.
[[nodiscard]] DocumentInfo AnalyzeDocument(std::wstring_view text, std::size_t previousPhysicalLineCount = 0);
// Normalize only newly inserted/replaced text to the document's first physical
// terminator; an empty or unterminated document uses CRLF. Existing source is not rewritten.
[[nodiscard]] std::wstring NormalizeInsertedLineEndings(std::wstring_view insertion, std::wstring_view document);
// Logarithmic read of the analyzed revision: no source scan or allocation.
// Oversized ACPs clamp to source end. An ACP between CR/LF stays at the prior
// line's content end; the following line begins only after the complete pair.
[[nodiscard]] DocumentPosition DocumentPositionAt(const DocumentInfo &info, std::size_t caret) noexcept;
} // namespace RedTilde
