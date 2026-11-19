#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "PrintPreview.h"
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
    if (printPreview || printOperation || printDialogOpen)
        return;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless && !options.printDialogForTest)
        return;
#else
    if (options.headless)
        return;
#endif
    Printing::Job job;
    job.document.source = editor->Snapshot();
    job.document.path = documentPath;
    job.document.title = documentPath.empty() ? Resource(IDS_UNTITLED) : documentPath.filename().native();
    job.document.extensions = editor->GetMarkdownAnalysisExtensions();
    job.document.allowRemoteImages = editor->RemoteImagesEnabled();
    job.document.strikeCompletedTasks = settings.strikeCompletedTasks;
    if (job.document.allowRemoteImages)
        job.document.networkImages = printNetworkImages;
    job.initialPaper = printPaper;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless)
    {
        if (!job.initialPaper)
            job.initialPaper = options.printPaperForTest;
        job.pageForTest = options.printPageForTest;
        job.finishForTest = options.printFinishForTest;
    }
#endif
    auto preview = std::make_unique<PrintPreview>(*this);
    auto result = preview->Initialize(job.document.title);
    if (SUCCEEDED(result))
    {
        printTitle = job.document.title;
        result = Printing::Start(window.get(), PrintReady, std::move(job), printOperation);
    }
    if (FAILED(result))
    {
        ReportPrintFailure(result, true);
        return;
    }
    printInvoker = host.GetFocusControl();
    if (printInvoker)
        printInvokerLifetime = GetControlLifetimeToken(*printInvoker);
    printPreview = std::move(preview);
    printPreview->Show();
}

void Application::PreviewPrintPage(unsigned number)
{
    if (!printOperation || !printPreview || !printPreview->CanPrint() || printDialogOpen || number == printPreview->Page().number)
        return;
    printPreview->BeginRendering();
    Printing::RequestPreview(*printOperation, number);
}

void Application::ChoosePrintDestination()
{
    if (!printOperation || !printPreview || !printPreview->CanPrint() || printDialogOpen || closing || closingAfterSave || sessionEnding)
        return;
    const auto operation = printOperation;
    const auto owner = printPreview->Window();
    auto &previewHost = printPreview->Host();
    auto *const focus = previewHost.GetFocusControl();
    const auto focusLifetime = focus ? GetControlLifetimeToken(*focus) : std::weak_ptr<int>{};
    const auto number = printPreview->Page().number;
    const auto count = printPreview->Page().count;
    const Printing::DialogOptions initial{.pageCount = count,
                                          .ranges = printRanges,
                                          .rangeNeedsCorrection = printRangeNeedsCorrection,
                                          .currentPage = printCurrentPage,
                                          .printToFile = printToFile};
    Printing::DialogResult choice;
    printDialogOpen = true;
    const auto restore = wil::scope_exit(
        [&]
        {
            printDialogOpen = false;
            if (focus && !focusLifetime.expired() && printOperation == operation && printPreview && printPreview->Window() == owner && !closing &&
                !sessionEnding && printPreview->CanPrint())
                printPreview->Host().SetFocusControl(focus, !options.headless && GetForegroundWindow() == owner);
        });
#if defined(REDTILDE_ENABLE_TESTS)
    const auto chosen = options.headless ? options.printDialogForTest(initial, choice) : printDialog.Choose(owner, initial, choice);
#else
    const auto chosen = printDialog.Choose(owner, initial, choice);
#endif
    if (printOperation != operation || !printPreview || printPreview->Window() != owner || closing || closingAfterSave || sessionEnding ||
        chosen == S_FALSE || chosen == Cancelled)
        return;
    if (FAILED(chosen))
    {
        ReportPrintFailure(chosen);
        return;
    }
    auto &selection = choice.selection;
    printPaper = selection.paper;
    printCurrentPage = choice.currentPage;
    printToFile = choice.printToFile;
    printPreviewPrinter = selection.printer;
    printRangeNeedsCorrection = false;
    if (selection.paper != printPreview->Page().paper || (!choice.confirmed && printPreview->Page().fallbackPaper))
    {
        printRanges = std::move(selection.ranges);
        printPaperChanged = true;
        printPreview->BeginRendering();
        Printing::RequestPreview(*operation, number, selection.paper);
        return;
    }
    if (!Printing::PageRangesFit(selection.ranges, count))
    {
        printRanges.clear();
        printRangeNeedsCorrection = true;
        printPreview->SetStatus(std::vformat(Resource(IDS_PRINTRANGE), std::make_wformat_args(count)));
        return;
    }
    if (!choice.confirmed)
    {
        printRanges = std::move(selection.ranges);
        printPreview->SetStatus(printPreviewPrinter);
        return;
    }
    if (choice.currentPage)
        selection.ranges = {{number, number}};
    ClosePrintPreview();
    Status(IDS_PRINTING);
    Printing::RequestPrint(*operation, std::move(selection));
}

void Application::ClosePrintPreview() noexcept
{
    if (!printPreview)
        return;
    printPreview.reset();
    if (!closing && !sessionEnding)
    {
        auto *const destination = printInvoker && !printInvokerLifetime.expired() ? printInvoker : editor;
        host.SetFocusControl(destination, MayRequestNativeFocus());
    }
    printInvoker = nullptr;
    printInvokerLifetime.reset();
    printRanges.clear();
    printPreviewPrinter.clear();
    printCurrentPage = printToFile = printPaperChanged = printRangeNeedsCorrection = false;
}

void Application::DismissPrintPreview() noexcept
{
    if (!printPreview || printDialogOpen)
        return;
    Printing::Stop(printOperation);
    ClosePrintPreview();
}

void Application::PrintCompleted()
{
    const auto operation = printOperation;
    auto notice = Printing::TakeNotice(operation);
    if (!notice || closing || sessionEnding)
        return;
    if (notice->kind == Printing::Notice::Kind::Preview)
    {
        if (!printPreview)
            return;
        auto &rendered = notice->preview.value();
        std::wstring status;
        if (!Printing::PageRangesFit(printRanges, rendered.count))
        {
            printRanges.clear();
            printRangeNeedsCorrection = true;
        }
        if (printRangeNeedsCorrection)
        {
            status = std::vformat(Resource(IDS_PRINTRANGE), std::make_wformat_args(rendered.count));
        }
        else if (printPaperChanged)
            status = std::vformat(Resource(IDS_PRINTPREVIEWCHANGED), std::make_wformat_args(printPreviewPrinter));
        else if (rendered.fallbackPaper)
            status = Resource(IDS_PRINTPREVIEWDEFAULT);
        printPreview->Present(std::move(rendered), std::move(status));
        return;
    }
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
    const bool previewFailed = static_cast<bool>(printPreview);
    ClosePrintPreview();
    if (notice->invalidRangePageCount)
    {
        const auto pages = notice->invalidRangePageCount;
        feedback->SetText(std::vformat(Resource(IDS_PRINTRANGE), std::make_wformat_args(pages)));
    }
    else if (notice->result == Cancelled)
        Status(IDS_PRINTCANCELLED);
    else if (FAILED(notice->result))
        ReportPrintFailure(notice->result, previewFailed);
    else
        Status(notice->savedFile ? IDS_PRINTSAVED : IDS_PRINTSUBMITTED);
}

void Application::ReportPrintFailure(HRESULT result, bool previewFailed)
{
    MonitorDiagnostics::Write(RedMonitor::Severity::Warning, std::format(L"Print failed (0x{:08X})", static_cast<unsigned>(result)));
    const bool unavailable = result == HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME) ||
                             result == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) || result == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
    const auto id = static_cast<unsigned>(previewFailed ? IDS_PRINTPREVIEWFAILED : unavailable ? IDS_PRINTNOPRINTER : IDS_PRINTFAILED);
    if (printPreview)
        printPreview->SetStatus(Resource(id));
    else
        Status(id);
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
