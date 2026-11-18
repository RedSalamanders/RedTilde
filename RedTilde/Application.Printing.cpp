#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "res/Resource.h"
#include <format>
#include <shobjidl.h>
#include <wil/result.h>

namespace RedTilde
{
namespace
{
constexpr HRESULT Cancelled = HRESULT_FROM_WIN32(ERROR_CANCELLED);

HRESULT PickPrintOutput(HWND owner, Printing::FileFormat format, std::wstring_view title, std::filesystem::path &path)
{
    using Printing::FileFormat;
    wil::com_ptr<IFileSaveDialog> dialog;
    RETURN_IF_FAILED(DxUi::Failure::InvokeNative(&CoCreateInstance, CLSID_FileSaveDialog, nullptr, static_cast<DWORD>(CLSCTX_INPROC_SERVER),
                                                 __uuidof(IFileSaveDialog), reinterpret_cast<void **>(dialog.put())));
    const auto label = Resource(static_cast<unsigned>(format == FileFormat::Pdf       ? IDS_PRINTPDF
                                                      : format == FileFormat::OpenXps ? IDS_PRINTXPS
                                                                                      : IDS_PRINTDRIVERFILE));
    const wchar_t *extension = format == FileFormat::Pdf ? L"pdf" : format == FileFormat::OpenXps ? L"oxps" : L"prn";
    const std::wstring pattern = std::format(L"*.{}", extension);
    const COMDLG_FILTERSPEC filter{label.c_str(), pattern.c_str()};
    const auto proposedName = std::filesystem::path(title).stem().native();
    DWORD flags{};
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::GetOptions, &flags));
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::SetOptions,
                                        flags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_NOCHANGEDIR));
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::SetTitle, Resource(IDS_PRINTSAVE).c_str()));
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::SetFileTypes, 1u, &filter));
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::SetDefaultExtension, extension));
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::SetFileName, proposedName.c_str()));
    // The normal cancellation HRESULT is handled by the caller as a dismissal.
    const auto shown = InvokeNativeMethod(dialog.get(), &IFileSaveDialog::Show, owner);
    if (FAILED(shown))
        return shown;
    wil::com_ptr<IShellItem> item;
    wil::unique_cotaskmem_string filename;
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::GetResult, item.put()));
    RETURN_IF_FAILED(InvokeNativeMethod(item.get(), &IShellItem::GetDisplayName, SIGDN_FILESYSPATH, filename.put()));
    path = filename.get();
    return path.empty() ? E_INVALIDARG : S_OK;
}
} // namespace

void Application::Print()
{
    if (printDialogOpen || printOperation)
        return;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless && !options.printDialogForTest)
        return;
#else
    if (options.headless)
        return;
#endif
    const HWND owner = window.get();
    auto *const focus = host.GetFocusControl();
    const auto focusLifetime = focus ? GetControlLifetimeToken(*focus) : std::weak_ptr<int>{};
    Printing::Job job;
    job.document.source = editor->Snapshot();
    job.document.path = documentPath;
    job.document.title = documentPath.empty() ? Resource(IDS_UNTITLED) : documentPath.filename().native();
    job.document.extensions = editor->GetMarkdownAnalysisExtensions();
    job.document.allowRemoteImages = editor->RemoteImagesEnabled();
    job.document.strikeCompletedTasks = settings.strikeCompletedTasks;
    if (job.document.allowRemoteImages)
        job.document.networkImages = printNetworkImages;
    printDialogOpen = true;
    const auto restore = wil::scope_exit(
        [&]
        {
            printDialogOpen = false;
            if (focus && !focusLifetime.expired() && window.get() == owner && !closing && !sessionEnding)
                host.SetFocusControl(focus, MayRequestNativeFocus());
        });
#if defined(REDTILDE_ENABLE_TESTS)
    const auto chosen = options.headless ? options.printDialogForTest(job.selection) : printDialog.Choose(owner, job.selection);
    if (options.headless)
    {
        job.pageForTest = options.printPageForTest;
        job.finishForTest = options.printFinishForTest;
    }
#else
    const auto chosen = printDialog.Choose(owner, job.selection);
#endif
    if (window.get() != owner || closing || closingAfterSave || sessionEnding || chosen == S_FALSE || chosen == Cancelled)
        return;
    if (FAILED(chosen))
    {
        ReportPrintFailure(chosen);
        return;
    }
    printTitle = job.document.title;
    const auto started = Printing::Start(owner, PrintReady, std::move(job), printOperation);
    if (FAILED(started))
        ReportPrintFailure(started);
    else
        Status(IDS_PRINTING);
}

void Application::PrintCompleted()
{
    const auto operation = printOperation;
    const auto notice = Printing::TakeNotice(operation);
    if (!notice || closing || sessionEnding)
        return;
    if (notice->kind == Printing::Notice::Kind::ChooseFile)
    {
        const HWND owner = window.get();
        auto *const focus = host.GetFocusControl();
        const auto focusLifetime = focus ? GetControlLifetimeToken(*focus) : std::weak_ptr<int>{};
        std::filesystem::path path;
        const HRESULT chosen = options.headless ? Cancelled : PickPrintOutput(owner, notice->format, printTitle, path);
        if (printOperation == operation && window.get() == owner && !closing && !sessionEnding)
        {
            Printing::ChooseFile(operation, chosen, std::move(path));
            if (focus && !focusLifetime.expired())
                host.SetFocusControl(focus, MayRequestNativeFocus());
        }
        return;
    }
    Printing::Stop(printOperation);
    if (notice->invalidRangePageCount)
    {
        const auto pages = notice->invalidRangePageCount;
        feedback->SetText(std::vformat(Resource(IDS_PRINTRANGE), std::make_wformat_args(pages)));
    }
    else if (notice->result == Cancelled)
        Status(IDS_PRINTCANCELLED);
    else if (FAILED(notice->result))
        ReportPrintFailure(notice->result);
    else
        Status(notice->savedFile ? IDS_PRINTSAVED : IDS_PRINTSUBMITTED);
}

void Application::ReportPrintFailure(HRESULT result)
{
    MonitorDiagnostics::Write(RedMonitor::Severity::Warning, std::format(L"Print failed (0x{:08X})", static_cast<unsigned>(result)));
    if (result == HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME) ||
        result == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        Status(IDS_PRINTNOPRINTER);
    else
        Status(IDS_PRINTFAILED);
}

void Application::RememberPrintImage(const DxUi::RichText::RichImageRequest &request, std::shared_ptr<const DxUi::RichText::RichImage> pixels)
{
    using DxUi::RichText::RichImageSourceKind;
    if (!pixels || (request.SourceKind() != RichImageSourceKind::RemoteHttp && request.SourceKind() != RichImageSourceKind::NetworkFile))
        return;
    constexpr std::size_t limit = 64u * 1024u * 1024u;
    std::erase_if(printNetworkImages, [&](const auto &image)
                  { return image.request.SourceKind() == request.SourceKind() && image.request.Source() == request.Source(); });
    std::size_t total = pixels->RetainedBytes();
    for (const auto &image : printNetworkImages)
        total += image.pixels->RetainedBytes();
    while (!printNetworkImages.empty() && (total > limit || printNetworkImages.size() >= RichImageLoader::MaximumResults))
    {
        total -= printNetworkImages.front().pixels->RetainedBytes();
        printNetworkImages.erase(printNetworkImages.begin());
    }
    if (total <= limit)
        printNetworkImages.push_back({request, std::move(pixels)});
}
} // namespace RedTilde
