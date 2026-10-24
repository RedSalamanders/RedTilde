#include "DocumentInfo.h"
#include <algorithm>
#include <cwctype>
#include <iterator>

namespace RedTilde
{
DocumentInfo AnalyzeDocument(std::wstring_view text, std::size_t previousPhysicalLineCount)
{
    DocumentInfo result;
    // Every source unit could be a terminator, followed by one empty line. The
    // vector bound also keeps length + 1 safe for any representable input view.
    const auto maximumLineCapacity = text.size() < result.lineIndex.max_size() ? text.size() + 1 : result.lineIndex.max_size();
    result.lineIndex.reserve(std::min(previousPhysicalLineCount, maximumLineCapacity));
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
        result.lineIndex.push_back({offset, (end == text.npos ? text.size() : end) - offset});
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
    return result;
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

std::wstring EquationAt(std::wstring_view text, std::size_t caret)
{
    // Deliberately bounded inspector extraction, not a Markdown parser. The selected
    // equation must enclose the caret; escaped dollars and code fences are ignored.
    bool fenced{};
    std::size_t opening = text.npos, delimiter{};
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if ((index == 0 || text[index - 1] == L'\n') && text.substr(index, 3) == L"```")
        {
            fenced = !fenced;
            opening = text.npos;
        }
        if (fenced || text[index] != L'$' || (index && text[index - 1] == L'\\'))
            continue;
        const std::size_t width = text.substr(index, 2) == L"$$" ? 2u : 1u;
        if (opening == text.npos)
        {
            opening = index;
            delimiter = width;
        }
        else if (width == delimiter)
        {
            if (caret >= opening && caret <= index + width && index - opening <= 2048)
                return std::wstring(text.substr(opening + width, index - opening - width));
            opening = text.npos;
        }
        index += width - 1;
    }
    return {};
}
} // namespace RedTilde
