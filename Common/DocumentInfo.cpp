#include "DocumentInfo.h"
#include <algorithm>
#include <cwctype>
#include <iterator>
#include <limits>
#include <utility>

namespace RedTilde
{
std::wstring NormalizeInsertedLineEndings(std::wstring_view insertion, std::wstring_view document)
{
    const auto first = document.find_first_of(L"\r\n");
    const auto eol = first == document.npos                                                                    ? std::wstring_view(L"\r\n")
                     : document[first] == L'\r' && first + 1 < document.size() && document[first + 1] == L'\n' ? document.substr(first, 2)
                                                                                                               : document.substr(first, 1);
    std::wstring normalized;
    normalized.reserve(insertion.size());
    for (std::size_t at = 0; at < insertion.size(); ++at)
    {
        const auto unit = insertion[at];
        if (unit != L'\r' && unit != L'\n')
            normalized.push_back(unit);
        else
        {
            normalized.append(eol);
            if (unit == L'\r' && at + 1 < insertion.size() && insertion[at + 1] == L'\n')
                ++at;
        }
    }
    return normalized;
}
namespace
{
constexpr std::size_t MaximumRetiredLineIndexCapacity = 16384;
std::size_t RetiredCapacityLimit(std::size_t newLineCount) noexcept
{
    const auto doubled = newLineCount > MaximumRetiredLineIndexCapacity / 2 ? MaximumRetiredLineIndexCapacity : newLineCount * 2;
    return std::min(MaximumRetiredLineIndexCapacity, std::max<std::size_t>(64, doubled));
}
void IncrementSaturated(std::size_t &value) noexcept
{
    if (value != std::numeric_limits<std::size_t>::max())
        ++value;
}
DocumentInfo AnalyzeDocumentImpl(std::wstring_view text, std::vector<DocumentLine> lineStorage, std::size_t previousPhysicalLineCount,
                                 DocumentLineIndexStorageStats *stats)
{
    DocumentInfo result;
    // Reserve hints affect capacity only; source length bounds them in both paths.
    const auto reusableCapacity = lineStorage.capacity();
    const auto maximumLineCapacity = text.size() < lineStorage.max_size() ? text.size() + 1 : lineStorage.max_size();
    lineStorage.reserve(std::min(previousPhysicalLineCount, maximumLineCapacity));
    if (stats && lineStorage.capacity() != reusableCapacity)
        IncrementSaturated(stats->capacityGrowthCount);
    result.lineIndex = std::move(lineStorage);
    result.lineIndex.clear();
    result.characters = text.size();
    bool word = false;
    for (wchar_t value : text)
    {
        const bool current = value <= 0x7f ? (value >= L'A' && value <= L'Z') || (value >= L'a' && value <= L'z') || (value >= L'0' && value <= L'9')
                                           : iswalnum(value) != 0 || value >= 0x2E80;
        if (current && !word)
            ++result.words;
        word = current;
    }

    wchar_t fence{};
    std::size_t fenceLength{};
    std::size_t offset{};
    bool crlf{}, cr{}, lf{};
    for (;;)
    {
        const auto end = text.find_first_of(L"\r\n", offset);
        const auto capacityBeforePush = result.lineIndex.capacity();
        result.lineIndex.push_back({offset, (end == text.npos ? text.size() : end) - offset});
        if (stats && result.lineIndex.capacity() != capacityBeforePush)
            IncrementSaturated(stats->capacityGrowthCount);
        auto line = text.substr(offset, end == text.npos ? text.size() - offset : end - offset);
        std::size_t indent{};
        while (indent < line.size() && indent < 4 && line[indent] == L' ')
            ++indent;
        line.remove_prefix(indent);
        if (indent < 4 && !line.empty() && (line.front() == L'`' || line.front() == L'~'))
        {
            const auto marker = line.front();
            std::size_t count{};
            while (count < line.size() && line[count] == marker)
                ++count;
            if (!fence && count >= 3)
            {
                fence = marker;
                fenceLength = count;
            }
            else if (fence == marker && count >= fenceLength && line.find_first_not_of(L" \t", count) == line.npos)
                fence = 0;
        }
        else if (!fence && indent < 4)
        {
            std::size_t count{};
            while (count < line.size() && line[count] == L'#')
                ++count;
            if (count && count <= 6 && (count == line.size() || iswspace(line[count])))
            {
                auto body = line.substr(count);
                while (!body.empty() && iswspace(body.front()))
                    body.remove_prefix(1);
                while (!body.empty() && iswspace(body.back()))
                    body.remove_suffix(1);
                const auto hashes = body.find_last_not_of(L'#');
                if (hashes != body.npos && hashes + 1 < body.size() && iswspace(body[hashes]))
                {
                    body = body.substr(0, hashes);
                    while (!body.empty() && iswspace(body.back()))
                        body.remove_suffix(1);
                }
                if (!body.empty() && result.headings.size() < 128)
                    result.headings.push_back({static_cast<unsigned>(count), offset + indent, std::wstring(body)});
            }
        }
        if (end == text.npos)
            break;
        offset = end + 1;
        if (text[end] == L'\r' && offset < text.size() && text[offset] == L'\n')
        {
            crlf = true;
            ++offset;
        }
        else if (text[end] == L'\r')
            cr = true;
        else
            lf = true;
    }
    result.lines = result.lineIndex.size();
    const auto kinds = static_cast<unsigned>(crlf) + static_cast<unsigned>(cr) + static_cast<unsigned>(lf);
    result.newlineKind = kinds > 1 ? DocumentNewlineKind::Mixed
                         : crlf    ? DocumentNewlineKind::CrLf
                         : cr      ? DocumentNewlineKind::Cr
                         : lf      ? DocumentNewlineKind::Lf
                                   : DocumentNewlineKind::None;
    if (stats)
    {
        const auto doubledActual =
            result.lines > std::numeric_limits<std::size_t>::max() / 2 ? std::numeric_limits<std::size_t>::max() : result.lines * 2;
        const auto activeCapacityLimit = std::max<std::size_t>(64, doubledActual);
        if (result.lineIndex.capacity() > activeCapacityLimit)
        {
            std::vector<DocumentLine> normalized;
            normalized.reserve(result.lines);
            normalized.insert(normalized.end(), result.lineIndex.begin(), result.lineIndex.end());
            result.lineIndex.swap(normalized);
            IncrementSaturated(stats->capacityNormalizationCount);
        }
        if (reusableCapacity >= result.lines)
            IncrementSaturated(stats->reuseCount);
        stats->activeCapacity = result.lineIndex.capacity();
    }
    return result;
}
} // namespace

DocumentInfo AnalyzeDocument(std::wstring_view text, std::size_t previousPhysicalLineCount)
{
    return AnalyzeDocumentImpl(text, {}, previousPhysicalLineCount, nullptr);
}

DocumentInfo DocumentLineIndexStorage::Analyze(std::wstring_view text, std::size_t previousPhysicalLineCount)
{
    auto reusable = std::move(spare);
    stats.spareCapacity = 0;
    return AnalyzeDocumentImpl(text, std::move(reusable), previousPhysicalLineCount, &stats);
}

void DocumentLineIndexStorage::Retire(std::vector<DocumentLine> &&previous, std::size_t activeLineCount) noexcept
{
    const auto capacityLimit = RetiredCapacityLimit(activeLineCount);
    if (previous.capacity() > capacityLimit)
    {
        std::vector<DocumentLine>{}.swap(previous);
        IncrementSaturated(stats.droppedSpareCount);
    }
    spare.swap(previous);
    stats.spareCapacity = spare.capacity();
}

DocumentPosition DocumentPositionAt(const DocumentInfo &info, std::size_t caret) noexcept
{
    if (info.lineIndex.empty())
        return {};
    caret = std::min(caret, info.characters);
    const auto after = std::upper_bound(info.lineIndex.begin(), info.lineIndex.end(), caret,
                                        [](std::size_t offset, const DocumentLine &line) { return offset < line.offset; });
    const auto current = after == info.lineIndex.begin() ? after : std::prev(after);
    return {static_cast<std::size_t>(current - info.lineIndex.begin()) + 1, std::min(caret - current->offset, current->length) + 1, current->offset,
            current->length};
}

} // namespace RedTilde
