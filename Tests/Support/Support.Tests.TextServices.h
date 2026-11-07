#pragma once
#include <DxUi/DxUi.h>
#include <textstor.h>
#include <type_traits>
#include <wrl/implements.h>

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

// Consumer-owned COM fixture: the SDK's private native test helpers are not a
// supported application dependency. Borrow its action only during RequestLock;
// the action records HRESULTs and cannot throw across this COM boundary.
class NativeTextStoreSink final
    : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, ITextStoreACPSink>
{
  public:
    NativeTextStoreSink() = default;
    NativeTextStoreSink(const NativeTextStoreSink &) = delete;
    NativeTextStoreSink &operator=(const NativeTextStoreSink &) = delete;
    NativeTextStoreSink(NativeTextStoreSink &&) = delete;
    NativeTextStoreSink &operator=(NativeTextStoreSink &&) = delete;
    HRESULT (*action)(void *) noexcept {};
    void *context{};
    HRESULT STDMETHODCALLTYPE OnTextChange(DWORD, const TS_TEXTCHANGE *) noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnSelectionChange() noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnLayoutChange(TsLayoutCode, TsViewCookie) noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnStatusChange(DWORD) noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnAttrsChange(LONG, LONG, ULONG, const TS_ATTRID *) noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnLockGranted(DWORD) noexcept override
    {
        return action ? action(context) : S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnStartEditTransaction() noexcept override
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnEndEditTransaction() noexcept override
    {
        return S_OK;
    }
};
} // namespace RedTilde::Testing
