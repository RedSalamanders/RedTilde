#pragma once
#include <DxUi/DxUi.h>
#include <type_traits>

namespace RedTilde::Testing
{
// Exercise the canonical text-service boundary on the actual editor. An
// inherited member pointer remains a base member pointer; no derived object,
// downcast, callback replacement or visible native IME session is involved.
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct TextServiceImportProbe : DxUi::RichTextEditor
{
    static constexpr auto Import = &TextServiceImportProbe::ImportTextInputState;
};
#pragma warning(pop)
static_assert(std::is_same_v<std::remove_cv_t<decltype(TextServiceImportProbe::Import)>,
                             bool (DxUi::RichTextEditor::*)(DxUi::ControlHost &, const DxUi::TextInputState &, bool)>);
} // namespace RedTilde::Testing
