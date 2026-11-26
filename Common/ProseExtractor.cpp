#include "ProseExtractor.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace RedTilde
{
namespace
{
constexpr std::size_t MaximumChunkUnits = 4096u;
constexpr wchar_t MaskSentinel          = L'\uFFFC';

[[nodiscard]] bool IsMaskOrSpace(wchar_t ch) noexcept
{
    return ch == MaskSentinel || ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

void MaskRange(std::wstring& buffer, std::size_t first, std::size_t last) noexcept
{
    const std::size_t clampedFirst = std::min(first, buffer.size());
    const std::size_t clampedLast  = std::min(last, buffer.size());
    for (std::size_t i = clampedFirst; i < clampedLast; ++i)
        buffer[i] = MaskSentinel;
}

void UnmaskRange(std::wstring& buffer, std::wstring_view source, std::size_t first, std::size_t last) noexcept
{
    const std::size_t clampedFirst = std::min({first, buffer.size(), source.size()});
    const std::size_t clampedLast  = std::min({last, buffer.size(), source.size()});
    for (std::size_t i = clampedFirst; i < clampedLast; ++i)
        buffer[i] = source[i];
}

void MaskAutolinksAndBareUrlsInBuffer(std::wstring& buffer) noexcept
{
    const std::size_t len = buffer.size();
    std::size_t i         = 0u;
    while (i < len)
    {
        while (i < len && IsMaskOrSpace(buffer[i]))
            ++i;
        if (i >= len)
            break;
        const std::size_t tokenStart = i;
        while (i < len && ! IsMaskOrSpace(buffer[i]))
            ++i;
        const std::wstring_view token(buffer.data() + tokenStart, i - tokenStart);
        if (LooksLikeInternetOrFilePath(buffer, tokenStart, token.size()))
            MaskRange(buffer, tokenStart, i);
    }
}

[[nodiscard]] std::vector<ProseChunk> ChunkMaskedBuffer(std::wstring_view buffer)
{
    std::vector<ProseChunk> chunks;
    const std::size_t len = buffer.size();
    std::size_t pos       = 0u;
    while (pos < len)
    {
        while (pos < len && IsMaskOrSpace(buffer[pos]))
            ++pos;
        if (pos >= len)
            break;

        std::size_t end = std::min(len, pos + MaximumChunkUnits);
        if (end < len)
        {
            // Adjust split backward to a mask/whitespace boundary so words never split across chunks.
            std::size_t split = end;
            while (split > pos && ! IsMaskOrSpace(buffer[split - 1u]))
                --split;
            if (split > pos)
                end = split;
            else
            {
                // Single >4,096-unit token: advance to its natural delimiter without breaking UTF-16 pairs.
                while (end < len && ! IsMaskOrSpace(buffer[end]))
                    ++end;
                if (end < len && IS_LOW_SURROGATE(buffer[end]))
                    ++end;
            }
        }
        while (end > pos && IsMaskOrSpace(buffer[end - 1u]))
            --end;
        if (end > pos)
            chunks.push_back(ProseChunk{pos, std::wstring(buffer.substr(pos, end - pos))});
        pos = end;
    }
    return chunks;
}

[[nodiscard]] bool LooksLikeEmailToken(std::wstring_view token) noexcept
{
    const std::size_t at = token.find(L'@');
    if (at == std::wstring_view::npos || at == 0u || at + 3u >= token.size())
        return false;
    const auto domain = token.substr(at + 1u);
    return domain.find(L'.') != std::wstring_view::npos && ! domain.starts_with(L'.') && ! domain.ends_with(L'.');
}

[[nodiscard]] bool LooksLikeWindowsOrUncPathToken(std::wstring_view token) noexcept
{
    if (token.size() >= 3u && ((token[0] >= L'A' && token[0] <= L'Z') || (token[0] >= L'a' && token[0] <= L'z')) && token[1] == L':' &&
        (token[2] == L'\\' || token[2] == L'/'))
        return true;
    if (token.starts_with(L"\\\\") && token.size() >= 5u && token.find(L'\\', 2u) != std::wstring_view::npos)
        return true;
    return false;
}

void StripSurroundingPunctuation(std::wstring_view& token) noexcept
{
    while (! token.empty() && (token.front() == L'(' || token.front() == L'[' || token.front() == L'<' || token.front() == L'"' || token.front() == L'\''))
        token.remove_prefix(1u);
    while (! token.empty() && (token.back() == L')' || token.back() == L']' || token.back() == L'>' || token.back() == L'"' || token.back() == L'\'' ||
                               token.back() == L'.' || token.back() == L',' || token.back() == L';' || token.back() == L':'))
        token.remove_suffix(1u);
}

void MaskInlineSyntaxNode(std::wstring& buffer,
                          const std::vector<DxUi::RichText::MarkdownInline>& inlines,
                          std::size_t index,
                          bool ignoreInternetAndPaths) noexcept
{
    using DxUi::RichText::MarkdownInlineKind;
    using DxUi::RichText::MarkdownNoNode;
    while (index != MarkdownNoNode && index < inlines.size())
    {
        const auto& node = inlines[index];
        if (node.kind == MarkdownInlineKind::Code || node.kind == MarkdownInlineKind::Html || node.kind == MarkdownInlineKind::Math ||
            node.kind == MarkdownInlineKind::FootnoteReference || node.kind == MarkdownInlineKind::SourceEditor || node.kind == MarkdownInlineKind::Image)
        {
            MaskRange(buffer, node.source.first, node.source.last);
        }
        else if (node.kind == MarkdownInlineKind::Link)
        {
            // Keep link label text unmasked inside children unless autolink; mask destination/title tail if present.
            if (node.firstChild != MarkdownNoNode)
            {
                std::size_t maxChildEnd = node.source.first;
                for (std::size_t child = node.firstChild; child != MarkdownNoNode && child < inlines.size(); child = inlines[child].nextSibling)
                    maxChildEnd = std::max(maxChildEnd, inlines[child].source.last);
                if (maxChildEnd < node.source.last)
                    MaskRange(buffer, maxChildEnd, node.source.last);
                // Mask leading '[' of [label](url).
                if (inlines[node.firstChild].source.first > node.source.first)
                    MaskRange(buffer, node.source.first, inlines[node.firstChild].source.first);
                MaskInlineSyntaxNode(buffer, inlines, node.firstChild, ignoreInternetAndPaths);
            }
            else if (ignoreInternetAndPaths)
            {
                MaskRange(buffer, node.source.first, node.source.last);
            }
        }
        else
        {
            if (node.firstChild != MarkdownNoNode)
                MaskInlineSyntaxNode(buffer, inlines, node.firstChild, ignoreInternetAndPaths);
        }
        index = node.nextSibling;
    }
}
} // namespace

bool LooksLikeInternetOrFilePath(std::wstring_view segment, std::size_t start, std::size_t length) noexcept
{
    if (start >= segment.size() || length == 0u || length > segment.size() - start)
        return false;

    std::size_t tokenStart = start;
    while (tokenStart > 0u && ! IsMaskOrSpace(segment[tokenStart - 1u]))
        --tokenStart;
    std::size_t tokenEnd = start + length;
    while (tokenEnd < segment.size() && ! IsMaskOrSpace(segment[tokenEnd]))
        ++tokenEnd;

    std::wstring_view token = segment.substr(tokenStart, tokenEnd - tokenStart);
    StripSurroundingPunctuation(token);
    if (token.empty())
        return false;

    if (StartsWithOrdinalIgnoreCase(token, L"http://") || StartsWithOrdinalIgnoreCase(token, L"https://") || StartsWithOrdinalIgnoreCase(token, L"mailto:") ||
        StartsWithOrdinalIgnoreCase(token, L"ftp://") || StartsWithOrdinalIgnoreCase(token, L"file://") || StartsWithOrdinalIgnoreCase(token, L"www."))
        return true;

    if (LooksLikeEmailToken(token) || LooksLikeWindowsOrUncPathToken(token))
        return true;

    return false;
}

bool ShouldSkipSpellError(std::wstring_view segment, SpellErrorSpan span, const SpellCheckOptions& options) noexcept
{
    if (span.start >= segment.size() || span.length == 0u || span.length > segment.size() - span.start)
        return true;
    if (span.kind == SpellIssueKind::RepeatedWord)
        return false;
    const std::wstring_view word = segment.substr(span.start, span.length);
    if (options.ignoreUppercase || options.ignoreWordsWithDigits)
    {
        bool isAllUpper = false;
        bool hasDigit   = false;
        AnalyzeWordCtype1(word, isAllUpper, hasDigit);
        if (options.ignoreUppercase && isAllUpper)
            return true;
        if (options.ignoreWordsWithDigits && hasDigit)
            return true;
    }
    if (options.ignoreInternetAndPaths && LooksLikeInternetOrFilePath(segment, span.start, span.length))
        return true;
    return false;
}

std::optional<std::wstring> ExtractFrontMatterLanguage(const DxUi::RichText::MarkdownSyntax* syntax, std::wstring_view sourceText)
{
    using namespace DxUi::RichText;
    if (! syntax || sourceText.empty())
        return std::nullopt;
    for (const auto& block : syntax->Blocks())
    {
        if (block.kind != MarkdownBlockKind::FrontMatter)
            continue;
        if (block.content.first >= block.content.last || block.content.last > sourceText.size())
            continue;
        const std::wstring_view body = sourceText.substr(block.content.first, block.content.last - block.content.first);
        std::size_t pos              = 0u;
        while (pos < body.size())
        {
            std::size_t lineEnd = body.find_first_of(L"\r\n", pos);
            if (lineEnd == std::wstring_view::npos)
                lineEnd = body.size();
            std::wstring_view line = body.substr(pos, lineEnd - pos);
            pos                    = lineEnd;
            while (pos < body.size() && (body[pos] == L'\r' || body[pos] == L'\n'))
                ++pos;

            // Only inspect top-level (column-0) keys so nested metadata maps do not spoof document language.
            if (line.empty() || line.front() == L' ' || line.front() == L'\t' || line.front() == L'#')
                continue;
            const bool isLangKey = line.starts_with(L"lang") || line.starts_with(L"language");
            if (! isLangKey)
                continue;
            const std::size_t keyLen = line.starts_with(L"language") ? 8u : 4u;
            std::wstring_view rest   = line.substr(keyLen);
            while (! rest.empty() && (rest.front() == L' ' || rest.front() == L'\t'))
                rest.remove_prefix(1u);
            if (rest.empty() || (rest.front() != L':' && rest.front() != L'='))
                continue;
            rest.remove_prefix(1u);
            while (! rest.empty() && (rest.front() == L' ' || rest.front() == L'\t'))
                rest.remove_prefix(1u);

            // Strip trailing inline YAML/TOML comment if unquoted.
            if (! rest.empty() && rest.front() != L'"' && rest.front() != L'\'')
            {
                if (const std::size_t comment = rest.find(L'#'); comment != std::wstring_view::npos)
                    rest = rest.substr(0u, comment);
            }
            while (! rest.empty() && (rest.back() == L' ' || rest.back() == L'\t'))
                rest.remove_suffix(1u);
            if (rest.size() >= 2u && ((rest.front() == L'"' && rest.back() == L'"') || (rest.front() == L'\'' && rest.back() == L'\'')))
            {
                rest.remove_prefix(1u);
                rest.remove_suffix(1u);
            }
            if (IsValidBcp47Tag(rest))
                return std::wstring(rest);
        }
    }
    return std::nullopt;
}

ExtractedProse ExtractSemanticProse(const DxUi::RichText::SemanticDocument& document,
                                    const DxUi::RichText::MarkdownSyntax* syntax,
                                    std::wstring_view sourceText,
                                    bool ignoreInternetAndPaths)
{
    using namespace DxUi::RichText;
    ExtractedProse result;
    result.semanticMode        = true;
    result.frontMatterLanguage = ExtractFrontMatterLanguage(syntax ? syntax : document.Syntax().get(), sourceText);

    const std::wstring_view display = document.Map().Display();
    if (display.empty())
        return result;

    std::wstring buffer(display);
    for (const auto& element : document.Elements())
    {
        if (element.role == SemanticRole::Code || element.role == SemanticRole::Html || element.role == SemanticRole::Metadata ||
            element.role == SemanticRole::Math || element.role == SemanticRole::Diagram || element.role == SemanticRole::Image)
        {
            MaskRange(buffer, element.display.first, element.display.last);
        }
        else if (ignoreInternetAndPaths && element.role == SemanticRole::Link)
        {
            if (element.display.first < element.display.last && element.display.last <= buffer.size())
            {
                const std::wstring_view label(display.data() + element.display.first, element.display.last - element.display.first);
                if (label == element.destination || StartsWithOrdinalIgnoreCase(label, L"http://") || StartsWithOrdinalIgnoreCase(label, L"https://") ||
                    StartsWithOrdinalIgnoreCase(label, L"www.") || StartsWithOrdinalIgnoreCase(label, L"mailto:"))
                    MaskRange(buffer, element.display.first, element.display.last);
            }
        }
    }

    for (const auto& run : document.Runs())
    {
        if ((run.flags & static_cast<unsigned>(Style::Code)) != 0u)
            MaskRange(buffer, run.start, run.start + run.length);
    }

    for (const auto& span : document.Map().Spans())
    {
        if (span.kind == DisplaySpanKind::Generated || span.kind == DisplaySpanKind::Object)
            MaskRange(buffer, span.display.first, span.display.last);
    }

    if (ignoreInternetAndPaths)
        MaskAutolinksAndBareUrlsInBuffer(buffer);
    result.chunks = ChunkMaskedBuffer(buffer);
    return result;
}

ExtractedProse ExtractSourceProse(const DxUi::RichText::MarkdownSyntax& syntax, std::wstring_view sourceText, bool ignoreInternetAndPaths)
{
    using namespace DxUi::RichText;
    ExtractedProse result;
    result.semanticMode        = false;
    result.frontMatterLanguage = ExtractFrontMatterLanguage(&syntax, sourceText);

    if (sourceText.empty())
        return result;

    std::wstring buffer(sourceText.size(), MaskSentinel);
    const auto& nodes   = syntax.Nodes();
    const auto& inlines = syntax.Inlines();
    for (const auto& node : nodes)
    {
        if (node.block.kind == MarkdownBlockKind::Paragraph || node.block.kind == MarkdownBlockKind::AtxHeading ||
            node.block.kind == MarkdownBlockKind::SetextHeading || node.block.kind == MarkdownBlockKind::TableCell)
        {
            for (const auto& line : node.lines)
                UnmaskRange(buffer, sourceText, line.source.first, line.source.last);
            if (node.firstInline != MarkdownNoNode)
                MaskInlineSyntaxNode(buffer, inlines, node.firstInline, ignoreInternetAndPaths);
        }
    }

    for (const auto& diag : syntax.Diagnostics())
        MaskRange(buffer, diag.source.first, diag.source.last);

    if (ignoreInternetAndPaths)
        MaskAutolinksAndBareUrlsInBuffer(buffer);
    result.chunks = ChunkMaskedBuffer(buffer);
    return result;
}

bool TryProjectDisplayErrorToSource(const DxUi::RichText::SemanticDocument& document,
                                    DxUi::RichText::MarkdownRange displayRange,
                                    std::vector<DxUi::RichText::MarkdownRange>& outSourceFragments)
{
    outSourceFragments.clear();
    if (displayRange.first >= displayRange.last)
        return false;
    if (document.Map().TrySourceRanges(displayRange, outSourceFragments) != DxUi::RichText::SourceResult::Success)
    {
        outSourceFragments.clear();
        return false;
    }
    std::erase_if(outSourceFragments, [](const DxUi::RichText::MarkdownRange& range) { return range.first >= range.last; });
    if (outSourceFragments.empty())
        return false;
    std::sort(outSourceFragments.begin(), outSourceFragments.end(), [](const DxUi::RichText::MarkdownRange& a, const DxUi::RichText::MarkdownRange& b) {
        return a.first < b.first;
    });
    const std::wstring_view source = document.Syntax() ? document.Syntax()->Source() : std::wstring_view{};
    const std::size_t sourceLen    = source.size();
    for (std::size_t i = 0u; i < outSourceFragments.size(); ++i)
    {
        const auto& frag = outSourceFragments[i];
        if (frag.last > sourceLen)
            return false;
        if (IS_LOW_SURROGATE(source[frag.first]) || (frag.last < sourceLen && IS_LOW_SURROGATE(source[frag.last])))
            return false;
        if (i > 0u && outSourceFragments[i - 1u].last > frag.first)
            return false;
    }
    return true;
}

bool TryBuildSpellReplacementEdits(const DxUi::RichText::SourceSnapshot& source,
                                   const std::vector<DxUi::RichText::MarkdownRange>& fragments,
                                   std::wstring_view replacement,
                                   std::vector<DxUi::RichText::SourceEdit>& outEdits)
{
    outEdits.clear();
    if (fragments.empty())
        return false;
    const std::size_t len = source.Length();
    for (std::size_t i = 0u; i < fragments.size(); ++i)
    {
        const auto& frag = fragments[i];
        if (frag.first >= frag.last || frag.last > len || ! source.IsBoundary(frag.first) || ! source.IsBoundary(frag.last))
            return false;
        if (i > 0u && fragments[i - 1u].last > frag.first)
            return false;
    }
    outEdits.reserve(fragments.size());
    for (std::size_t i = 0u; i < fragments.size(); ++i)
        outEdits.push_back(DxUi::RichText::SourceEdit{fragments[i], i == 0u ? replacement : std::wstring_view{}});
    return true;
}
} // namespace RedTilde
