#pragma once
#include <DxUi/DxUi.h>
#include <type_traits>

namespace RedTilde::Testing
{
// Exercise the canonical text-service boundary on the actual editor. An
// inherited member pointer remains a base member pointer; no derived object,
// downcast, callback replacement or visible native IME session is involved.
struct TextServiceImportProbe : DxUi::RichTextEditor
{
    static constexpr auto Import = &TextServiceImportProbe::ImportTextInputState;
};
static_assert(std::is_same_v<std::remove_cv_t<decltype(TextServiceImportProbe::Import)>,
                             bool (DxUi::RichTextEditor::*)(DxUi::ControlHost &, const DxUi::TextInputState &, bool)>);
} // namespace RedTilde::Testing
