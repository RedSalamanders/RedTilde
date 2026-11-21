#include "Application.h"
#include "Common/MonitorDiagnostics.h"
#include "Common/NativeCalls.h"
#include "PrintPreview.h"
#include "res/Resource.h"
#include <format>
#include <shobjidl.h>
#include <utility>
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
    if (!CanAcceptCommand() || HasBlockDraft() || HasActiveModalPrompt() || printPreview || printOperation)
        return;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless && !options.printDestinationsForTest)
        return;
#else
    if (options.headless)
        return;
#endif
    Printing::Job job;
    job.document.source = editor->Snapshot();
    job.document.path = session.path;
    job.document.title = session.path.empty() ? Resource(IDS_UNTITLED) : session.path.filename().native();
    job.document.extensions = editor->GetMarkdownAnalysisExtensions();
    job.document.allowRemoteImages = editor->RemoteImagesEnabled();
    job.document.strikeCompletedTasks = settings.strikeCompletedTasks;
    if (job.document.allowRemoteImages)
        job.document.networkImages = printNetworkImages;
    job.initialPrinter = printPrinter;
    job.initialSettings = printSettings;
#if defined(REDTILDE_ENABLE_TESTS)
    if (options.headless)
    {
        job.destinationsForTest = options.printDestinationsForTest.value_or(Printing::Destinations{});
        job.printerForTest = options.printPrinterForTest;
        job.pageForTest = options.printPageForTest;
        job.finishForTest = options.printFinishForTest;
        job.submissionForTest = options.printSubmissionForTest;
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
        ReportPrintFailure(result, Printing::Stage::Preview);
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
    if (!CanAcceptCommand() || !printOperation || !printPreview || !printPreview->IsReady() || number == printPreview->Page().number)
        return;
    printPreview->BeginRendering();
    Printing::RequestPreview(*printOperation, number);
}

void Application::PreviewPrintDestination(std::optional<Printing::PrinterSettings> requested)
{
    if (!CanAcceptCommand() || !printOperation || !printPreview || !printPreview->IsReady())
        return;
    auto printer = std::wstring(printPreview->Printer());
    if (!requested && Printing::SamePrintName(printer, printPrinter))
        requested = printSettings;
    const auto pageNumber = printPreview->Page().number;
    printPreview->BeginRendering();
    Printing::RequestPrinter(*printOperation, pageNumber, std::move(printer), std::move(requested));
}

void Application::SubmitPrintPreview()
{
    if (!CanAcceptCommand() || !printOperation || !printPreview || !printConfiguration || !printPreview->CanPrint())
        return;
    auto ranges = printPreview->SelectedRanges();
    if (!ranges)
    {
        printPreview->ReportInvalidRange(true);
        return;
    }
    const auto copies = printPreview->SelectedCopies(printConfiguration->maximumCopies);
    if (!copies)
        return;
    auto selection = std::move(printConfiguration->selection);
    printPrinter = selection.printer;
    printSettings = std::move(printConfiguration->settings);
    printSettings->copies = copies.value();
    if (printConfiguration->maximumCopies > 1u)
    {
        selection.copies = printSettings->copies;
        if (printSettings->collate)
        {
            printSettings->collate = printPreview->Collate();
            selection.collate = printSettings->collate;
        }
    }
    if (printPreview->PrintToFile())
        selection.port = L"FILE:";
    selection.ranges = std::move(ranges.value());
    ClosePrintPreview();
    Status(IDS_PRINTING);
    Printing::RequestPrint(*printOperation, std::move(selection));
}

void Application::ClosePrintPreview() noexcept
{
    if (!printPreview)
        return;
    printPreview.reset();
    if (!IsClosing() && !IsSessionEnding() && !IsClosingAfterSave())
    {
        auto *const destination = printInvoker && !printInvokerLifetime.expired() ? printInvoker : editor;
        host.SetFocusControl(destination, MayRequestNativeFocus());
    }
    printInvoker = nullptr;
    printInvokerLifetime.reset();
    printConfiguration.reset();
}

void Application::DismissPrintPreview() noexcept
{
    if (!printPreview)
        return;
    Printing::Stop(printOperation);
    ClosePrintPreview();
}

void Application::PrintCompleted()
{
    const auto operation = printOperation;
    auto notice = Printing::TakeNotice(operation);
    if (!notice || IsClosing() || IsSessionEnding() || IsClosingAfterSave())
    {
        if (notice && notice->kind == Printing::Notice::Kind::ChooseFile)
        {
            Printing::ChooseFile(operation, Cancelled, {});
            if (printOperation == operation)
                Printing::Stop(printOperation);
        }
        return;
    }
    if (notice->kind == Printing::Notice::Kind::Preview)
    {
        if (!printPreview)
            return;
        auto &rendered = notice->preview.value();
        if (notice->destinations)
            printPreview->SetDestinations(std::move(notice->destinations.value()));
        std::wstring status;
        if (notice->stage == Printing::Stage::ConfigurePrinter)
        {
            const bool samePrinter = printConfiguration && Printing::SamePrintName(printConfiguration->selection.printer, notice->printer);
            if (notice->configuration)
                printConfiguration = std::move(notice->configuration);
            else if (!samePrinter)
                printConfiguration.reset();
            printPreview->SetConfiguration(printConfiguration ? &printConfiguration.value() : nullptr, samePrinter);
            if (FAILED(notice->result) && !notice->printer.empty())
            {
                const auto summary = Resource(rendered.fallbackPaper ? IDS_PRINTPREVIEWDEFAULT : IDS_PRINTSETTINGSFAILED);
                const auto error = ErrorText(notice->result);
                status = FormatStringResource(nullptr, IDS_PRINTERRORDETAIL, summary, error);
            }
        }
        if (status.empty() && rendered.fallbackPaper)
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
        if (printOperation == operation && window.get() == owner && !IsClosing() && !IsSessionEnding() && !IsClosingAfterSave())
        {
            Printing::ChooseFile(operation, chosen, std::move(path));
            if (focus && !focusLifetime.expired())
                host.SetFocusControl(focus, MayRequestNativeFocus());
        }
        else
        {
            Printing::ChooseFile(operation, Cancelled, {});
            if (printOperation == operation)
                Printing::Stop(printOperation);
        }
        return;
    }
    Printing::Stop(printOperation);
    ClosePrintPreview();
    if (notice->invalidRangePageCount)
    {
        const auto pages = notice->invalidRangePageCount;
        feedback->SetText(FormatStringResource(nullptr, IDS_PRINTRANGE, pages));
    }
    else if (notice->result == Cancelled)
        Status(IDS_PRINTCANCELLED);
    else if (FAILED(notice->result))
        ReportPrintFailure(notice->result, notice->stage, notice->printer, notice->failedPage);
    else
        Status(notice->savedFile ? IDS_PRINTSAVED : IDS_PRINTSUBMITTED);
}

void Application::ReportPrintFailure(HRESULT result, Printing::Stage stage, std::wstring_view printer, unsigned pageNumber)
{
    using Printing::Stage;
    MonitorDiagnostics::Write(RedMonitor::Severity::Warning,
                              std::format(L"Print failed at stage {} (0x{:08X})", static_cast<unsigned>(stage), static_cast<unsigned>(result)));
    const bool unavailable = result == HRESULT_FROM_WIN32(ERROR_PRINTER_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME) ||
                             result == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) || result == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
    std::wstring summary;
    switch (stage)
    {
    case Stage::Preview:
        summary = Resource(IDS_PRINTPREVIEWFAILED);
        break;
    case Stage::ConfigurePrinter:
        summary = Resource(unavailable ? IDS_PRINTNOPRINTER : IDS_PRINTSETTINGSFAILED);
        break;
    case Stage::ChooseFile:
        summary = Resource(IDS_PRINTFILEDIALOGFAILED);
        break;
    case Stage::PrepareJob:
        summary = Resource(unavailable ? IDS_PRINTNOPRINTER : IDS_PRINTFAILED);
        break;
    case Stage::Page:
        summary = FormatStringResource(nullptr, IDS_PRINTPAGEFAILED, pageNumber);
        break;
    case Stage::FinishJob:
        summary = Resource(IDS_PRINTFINISHFAILED);
        break;
    case Stage::SaveFile:
        summary = Resource(IDS_PRINTFILEFAILED);
        break;
    }
    const auto messageSummary = printer.empty() ? summary : FormatStringResource(nullptr, IDS_PRINTDESTINATIONFAILED, summary, printer);
    const auto error = ErrorText(result);
    const auto message = FormatStringResource(nullptr, IDS_PRINTERRORDETAIL, messageSummary, error);
    feedback->SetText(messageSummary);
    if (printPreview)
        printPreview->SetStatus(messageSummary);
    if (!options.headless)
    {
        const HWND owner = printPreview ? printPreview->Window() : window.get();
        (void)DxUi::Failure::InvokeNative(&MessageBoxW, owner, message.c_str(), Resource(IDS_ERROR).c_str(), static_cast<UINT>(MB_OK | MB_ICONERROR));
    }
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
