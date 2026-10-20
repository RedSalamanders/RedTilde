#include "DocumentInfo.h"
#include <algorithm>
#include <cwctype>

namespace RedTilde
{
DocumentInfo AnalyzeDocument(std::wstring_view text)
{
    DocumentInfo result;
    result.characters = text.size();
    bool word = false;
    for (wchar_t value : text)
    {
        const bool current = iswalnum(value) != 0 || value >= 0x2E80;
        if (current && !word)
            ++result.words;
        word = current;
    }
    wchar_t fence{};
    std::size_t fenceLength{};
    std::size_t offset{};
    while (offset < text.size())
    {
        ++result.lines;
        const auto end = text.find_first_of(L"\r\n", offset);
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
                if (result.headings.size() < 128)
                    result.headings.push_back({static_cast<unsigned>(count), offset + indent, std::wstring(body)});
            }
        }
        if (end == text.npos)
            break;
        offset = end + 1;
        if (text[end] == L'\r' && offset < text.size() && text[offset] == L'\n')
            ++offset;
    }
    return result;
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
