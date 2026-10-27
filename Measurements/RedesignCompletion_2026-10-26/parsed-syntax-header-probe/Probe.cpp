#include <DxUi/DxUi.h>
#include <DxUi/MarkdownSyntax.h>
#include <DxUi/RichDocument.h>

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

#ifndef DXUI_PROBE_EXPECT_PARSED_ADOPTION
#error DXUI_PROBE_EXPECT_PARSED_ADOPTION must be set to 0 for baseline or 1 for candidate.
#endif
static_assert(DXUI_PROBE_EXPECT_PARSED_ADOPTION == 0 || DXUI_PROBE_EXPECT_PARSED_ADOPTION == 1);

using SourceSnapshot = DxUi::RichText::SourceSnapshot;
using MarkdownSyntax = DxUi::RichText::MarkdownSyntax;
using MarkdownParser = DxUi::RichText::MarkdownParser;
using MarkdownProfile = DxUi::RichText::MarkdownProfile;
using MarkdownExtensions = DxUi::RichText::MarkdownExtensions;
using Selection = DxUi::RichText::Selection;
using RichTextEditor = DxUi::RichTextEditor;

using ReadOnlySourceSynchronization = bool (RichTextEditor::*)(const SourceSnapshot&, Selection);
using SourceSnapshotMatchesText = bool (SourceSnapshot::*)(std::wstring_view) const noexcept;
using ParsedSnapshotAdoption = bool (MarkdownParser::*)(const SourceSnapshot&,
                                                         std::uint64_t,
                                                         const std::shared_ptr<const MarkdownSyntax>&,
                                                         MarkdownProfile,
                                                         std::optional<MarkdownExtensions>);
using ReadOnlyMarkdownSyntaxAdoption = HRESULT (RichTextEditor::*)(
    const SourceSnapshot&, const std::shared_ptr<const MarkdownSyntax>&) noexcept;

template <typename T>
concept HasExactReadOnlySourceSynchronization = requires {
    static_cast<ReadOnlySourceSynchronization>(&T::SynchronizeReadOnlySourceSnapshot);
};

template <typename T>
concept HasExactSourceSnapshotMatchesText = requires {
    static_cast<SourceSnapshotMatchesText>(&T::MatchesText);
};

template <typename T>
concept HasExactParsedSnapshotAdoption = requires {
    static_cast<ParsedSnapshotAdoption>(&T::TryAdoptParsedSnapshot);
};

template <typename T>
concept HasExactReadOnlyMarkdownSyntaxAdoption = requires {
    static_cast<ReadOnlyMarkdownSyntaxAdoption>(&T::TryAdoptReadOnlyMarkdownSyntax);
};

static_assert(HasExactReadOnlySourceSynchronization<RichTextEditor>);
static_assert(std::is_same_v<decltype(static_cast<ReadOnlySourceSynchronization>(
                  &RichTextEditor::SynchronizeReadOnlySourceSnapshot)),
              ReadOnlySourceSynchronization>);
static_assert(HasExactSourceSnapshotMatchesText<SourceSnapshot> == (DXUI_PROBE_EXPECT_PARSED_ADOPTION != 0));
static_assert(HasExactParsedSnapshotAdoption<MarkdownParser> == (DXUI_PROBE_EXPECT_PARSED_ADOPTION != 0));
static_assert(HasExactReadOnlyMarkdownSyntaxAdoption<RichTextEditor> == (DXUI_PROBE_EXPECT_PARSED_ADOPTION != 0));

#if DXUI_PROBE_EXPECT_PARSED_ADOPTION
static_assert(std::is_same_v<decltype(static_cast<SourceSnapshotMatchesText>(&SourceSnapshot::MatchesText)),
              SourceSnapshotMatchesText>);
static_assert(std::is_same_v<decltype(static_cast<ParsedSnapshotAdoption>(&MarkdownParser::TryAdoptParsedSnapshot)),
              ParsedSnapshotAdoption>);
static_assert(std::is_same_v<decltype(static_cast<ReadOnlyMarkdownSyntaxAdoption>(
                  &RichTextEditor::TryAdoptReadOnlyMarkdownSyntax)),
              ReadOnlyMarkdownSyntaxAdoption>);
#endif

static_assert(sizeof(MarkdownSyntax) > 0);
static_assert(sizeof(MarkdownParser) > 0);
static_assert(sizeof(RichTextEditor) > 0);
static_assert(sizeof(SourceSnapshot) > 0);
static_assert(sizeof(DxUi::RichText::Document) > 0);
static_assert(alignof(MarkdownSyntax) > 0);
static_assert(alignof(MarkdownParser) > 0);
static_assert(alignof(RichTextEditor) > 0);
static_assert(alignof(SourceSnapshot) > 0);
static_assert(alignof(DxUi::RichText::Document) > 0);

#if defined(_ITERATOR_DEBUG_LEVEL)
constexpr int iteratorDebugLevel = _ITERATOR_DEBUG_LEVEL;
#else
constexpr int iteratorDebugLevel = -1;
#endif

int main()
{
    std::printf(
        "{\"sizeofMarkdownSyntax\":%zu,\"alignofMarkdownSyntax\":%zu,"
        "\"sizeofMarkdownParser\":%zu,\"alignofMarkdownParser\":%zu,"
        "\"sizeofRichTextEditor\":%zu,\"alignofRichTextEditor\":%zu,"
        "\"sizeofSourceSnapshot\":%zu,\"alignofSourceSnapshot\":%zu,"
        "\"sizeofRichTextDocument\":%zu,\"alignofRichTextDocument\":%zu,"
        "\"hasReadOnlySourceSynchronization\":%s,"
        "\"hasSourceSnapshotMatchesText\":%s,"
        "\"hasParsedSnapshotAdoption\":%s,"
        "\"hasReadOnlyMarkdownSyntaxAdoption\":%s,"
        "\"controlIsPolymorphic\":%s,\"richTextEditorIsPolymorphic\":%s,"
        "\"msvcVersion\":%d,\"msvcLang\":%ld,\"iteratorDebugLevel\":%d}\n",
        sizeof(MarkdownSyntax),
        alignof(MarkdownSyntax),
        sizeof(MarkdownParser),
        alignof(MarkdownParser),
        sizeof(RichTextEditor),
        alignof(RichTextEditor),
        sizeof(SourceSnapshot),
        alignof(SourceSnapshot),
        sizeof(DxUi::RichText::Document),
        alignof(DxUi::RichText::Document),
        HasExactReadOnlySourceSynchronization<RichTextEditor> ? "true" : "false",
        HasExactSourceSnapshotMatchesText<SourceSnapshot> ? "true" : "false",
        HasExactParsedSnapshotAdoption<MarkdownParser> ? "true" : "false",
        HasExactReadOnlyMarkdownSyntaxAdoption<RichTextEditor> ? "true" : "false",
        std::is_polymorphic_v<DxUi::Control> ? "true" : "false",
        std::is_polymorphic_v<RichTextEditor> ? "true" : "false",
#if defined(_MSC_VER)
        _MSC_VER,
#else
        0,
#endif
        static_cast<long>(_MSVC_LANG),
        iteratorDebugLevel);
    return 0;
}
