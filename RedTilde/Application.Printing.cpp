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

HRESULT PickPrintOutput(HWND owner, Printing::FileFormat format, std::wstring_view title, std::filesystem::path& path)
{
    using Printing::FileFormat;
    wil::com_ptr<IFileSaveDialog> dialog;
    RETURN_IF_FAILED(DxUi::Failure::InvokeNative(&CoCreateInstance,
                                                 CLSID_FileSaveDialog,
                                                 nullptr,
                                                 static_cast<DWORD>(CLSCTX_INPROC_SERVER),
                                                 __uuidof(IFileSaveDialog),
                                                 reinterpret_cast<void**>(dialog.put())));
    const auto label           = Resource(static_cast<unsigned>(format == FileFormat::Pdf       ? IDS_PRINTPDF
                                                                : format == FileFormat::OpenXps ? IDS_PRINTXPS
                                                                                                : IDS_PRINTDRIVERFILE));
    const wchar_t* extension   = format == FileFormat::Pdf ? L"pdf" : format == FileFormat::OpenXps ? L"oxps" : L"prn";
    const std::wstring pattern = std::format(L"*.{}", extension);
    const COMDLG_FILTERSPEC filter{label.c_str(), pattern.c_str()};
    const auto proposedName = std::filesystem::path(title).stem().native();
    DWORD flags{};
    RETURN_IF_FAILED(InvokeNativeMethod(dialog.get(), &IFileSaveDialog::GetOptions, &flags));
    RETURN_IF_FAILED(InvokeNativeMethod(
        dialog.get(), &IFileSaveDialog::SetOptions, flags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_NOCHANGEDIR));
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
    if (! CanAcceptCommand() || HasBlockDraft() || HasActiveModalPrompt() || _printPreview || _printOperation)
        return;
#if defined(REDTILDE_ENABLE_TESTS)
    if (_options.headless && ! _options.printDestinationsForTest)
        return;
#else
    if (_options.headless)
        return;
#endif
    Printing::Job job;
    job.document.source               = _editor->Snapshot();
    job.document.path                 = _session.path;
    job.document.title                = _session.path.empty() ? Resource(IDS_UNTITLED) : _session.path.filename().native();
    job.document.extensions           = _editor->GetMarkdownAnalysisExtensions();
    job.document.allowRemoteImages    = _editor->RemoteImagesEnabled();
    job.document.strikeCompletedTasks = _settings.strikeCompletedTasks;
    if (job.document.allowRemoteImages)
        job.document.networkImages = _printNetworkImages;
    job.initialPrinter  = _printPrinter;
    job.initialSettings = _printSettings;
#if defined(REDTILDE_ENABLE_TESTS)
    job.printerTimeoutMsForTest = _options.printPrinterTimeoutMsForTest;
    if (_options.headless)
    {
        job.destinationsForTest = _options.printDestinationsForTest.value_or(Printing::Destinations{});
        job.printerForTest      = _options.printPrinterForTest;
        job.pageForTest         = _options.printPageForTest;
        job.finishForTest       = _options.printFinishForTest;
        job.submissionForTest   = _options.printSubmissionForTest;
    }
#endif
    auto preview = std::make_unique<PrintPreview>(*this);
    auto result  = preview->Initialize(job.document.title);
    if (SUCCEEDED(result))
    {
        _printTitle = job.document.title;
        result      = Printing::Start(_window.get(), PrintReady, std::move(job), _printOperation);
    }
    if (FAILED(result))
    {
        ReportPrintFailure(result, Printing::Stage::Preview);
        return;
    }
    _printInvoker = _host.GetFocusControl();
    if (_printInvoker)
        _printInvokerLifetime = GetControlLifetimeToken(*_printInvoker);
    _printPreview = std::move(preview);
    _printPreview->Show();
}

void Application::PreviewPrintPage(unsigned number)
{
    if (! CanAcceptCommand() || ! _printOperation || ! _printPreview || ! _printPreview->IsReady() || number == _printPreview->Page().number)
        return;
    _printPreview->BeginRendering();
    Printing::RequestPreview(*_printOperation, number);
}

void Application::PreviewPrintDestination(std::optional<Printing::PrinterSettings> requested)
{
    if (! CanAcceptCommand() || ! _printOperation || ! _printPreview || (requested && ! _printPreview->IsReady()))
        return;
    auto printer = std::wstring(_printPreview->Printer());
    if (printer.empty())
        return;
    if (! requested && Printing::SamePrintName(printer, _printPrinter))
        requested = _printSettings;
    if (! requested && _printConfiguration && ! Printing::SamePrintName(_printConfiguration->selection.printer, printer))
    {
        _printConfiguration.reset();
        _printPreview->SetConfiguration(nullptr, false);
    }
    const auto pageNumber = std::max(1u, _printPreview->Page().number);
    _printPreview->BeginRendering(FormatStringResource(nullptr, IDS_PRINTPREVIEWCONNECTING, printer));
    Printing::RequestPrinter(*_printOperation, pageNumber, std::move(printer), std::move(requested));
}

void Application::SubmitPrintPreview()
{
    if (! CanAcceptCommand() || ! _printOperation || ! _printPreview || ! _printConfiguration || ! _printPreview->CanPrint())
        return;
    auto ranges = _printPreview->SelectedRanges();
    if (! ranges)
    {
        _printPreview->ReportInvalidRange(true);
        return;
    }
    const auto copies = _printPreview->SelectedCopies(_printConfiguration->maximumCopies);
    if (! copies)
        return;
    auto selection         = std::move(_printConfiguration->selection);
    _printPrinter          = selection.printer;
    _printSettings         = std::move(_printConfiguration->settings);
    _printSettings->copies = copies.value();
    if (_printConfiguration->maximumCopies > 1u)
    {
        selection.copies = _printSettings->copies;
        if (_printSettings->collate)
        {
            _printSettings->collate = _printPreview->Collate();
            selection.collate       = _printSettings->collate;
        }
    }
    if (_printPreview->PrintToFile())
        selection.port = L"FILE:";
    selection.ranges = std::move(ranges.value());
    ClosePrintPreview();
    Status(IDS_PRINTING);
    Printing::RequestPrint(*_printOperation, std::move(selection));
}

void Application::ClosePrintPreview() noexcept
{
    if (! _printPreview)
        return;
    _printPreview.reset();
    if (! IsClosing() && ! IsSessionEnding() && ! IsClosingAfterSave())
    {
        auto* const destination = _printInvoker && ! _printInvokerLifetime.expired() ? _printInvoker : _editor;
        _host.SetFocusControl(destination, MayRequestNativeFocus());
    }
    _printInvoker = nullptr;
    _printInvokerLifetime.reset();
    _printConfiguration.reset();
}

void Application::DismissPrintPreview() noexcept
{
    if (! _printPreview)
        return;
    Printing::Stop(_printOperation);
    ClosePrintPreview();
}

void Application::PrintCompleted()
{
    const auto operation = _printOperation;
    auto notice          = Printing::TakeNotice(operation);
    if (! notice || IsClosing() || IsSessionEnding() || IsClosingAfterSave())
    {
        if (notice && notice->kind == Printing::Notice::Kind::ChooseFile)
        {
            Printing::ChooseFile(operation, Cancelled, {});
            if (_printOperation == operation)
                Printing::Stop(_printOperation);
        }
        return;
    }
    if (notice->kind == Printing::Notice::Kind::Destinations)
    {
        if (! _printPreview)
            return;
        if (notice->destinations)
            _printPreview->SetDestinations(std::move(notice->destinations.value()));
        if (! notice->printer.empty())
            _printPreview->SetStatus(FormatStringResource(nullptr, IDS_PRINTPREVIEWCONNECTING, notice->printer));
        return;
    }
    if (notice->kind == Printing::Notice::Kind::Preview)
    {
        if (! _printPreview)
            return;
        auto& rendered = notice->preview.value();
        if (notice->destinations)
            _printPreview->SetDestinations(std::move(notice->destinations.value()));
        std::wstring status;
        if (notice->stage == Printing::Stage::ConfigurePrinter)
        {
            const bool samePrinter = _printConfiguration && Printing::SamePrintName(_printConfiguration->selection.printer, notice->printer);
            if (notice->configuration)
                _printConfiguration = std::move(notice->configuration);
            else if (! samePrinter || notice->printerPending)
                _printConfiguration.reset();
            _printPreview->SetConfiguration(_printConfiguration ? &_printConfiguration.value() : nullptr, samePrinter && ! notice->printerPending);
            if (notice->printerPending && ! notice->printer.empty())
                status = FormatStringResource(nullptr, IDS_PRINTPREVIEWSLOW, notice->printer);
            else if (FAILED(notice->result) && ! notice->printer.empty())
            {
                const auto summary = Resource(rendered.fallbackPaper ? IDS_PRINTPREVIEWDEFAULT : IDS_PRINTSETTINGSFAILED);
                const auto error   = ErrorText(notice->result);
                status             = FormatStringResource(nullptr, IDS_PRINTERRORDETAIL, summary, error);
            }
        }
        if (status.empty() && notice->printerPending && ! notice->printer.empty())
            status = FormatStringResource(nullptr, IDS_PRINTPREVIEWSLOW, notice->printer);
        else if (status.empty() && rendered.fallbackPaper)
            status = Resource(IDS_PRINTPREVIEWDEFAULT);
        _printPreview->Present(std::move(rendered), std::move(status));
        return;
    }
    if (notice->kind == Printing::Notice::Kind::ChooseFile)
    {
        const HWND owner         = _window.get();
        auto* const focus        = _host.GetFocusControl();
        const auto focusLifetime = focus ? GetControlLifetimeToken(*focus) : std::weak_ptr<int>{};
        std::filesystem::path path;
        const HRESULT chosen = _options.headless ? Cancelled : PickPrintOutput(owner, notice->format, _printTitle, path);
        if (_printOperation == operation && _window.get() == owner && ! IsClosing() && ! IsSessionEnding() && ! IsClosingAfterSave())
        {
            Printing::ChooseFile(operation, chosen, std::move(path));
            if (focus && ! focusLifetime.expired())
                _host.SetFocusControl(focus, MayRequestNativeFocus());
        }
        else
        {
            Printing::ChooseFile(operation, Cancelled, {});
            if (_printOperation == operation)
                Printing::Stop(_printOperation);
        }
        return;
    }
    Printing::Stop(_printOperation);
    ClosePrintPreview();
    if (notice->invalidRangePageCount)
    {
        const auto pages = notice->invalidRangePageCount;
        _feedback->SetText(FormatStringResource(nullptr, IDS_PRINTRANGE, pages));
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
        case Stage::Preview: summary = Resource(IDS_PRINTPREVIEWFAILED); break;
        case Stage::ConfigurePrinter: summary = Resource(unavailable ? IDS_PRINTNOPRINTER : IDS_PRINTSETTINGSFAILED); break;
        case Stage::ChooseFile: summary = Resource(IDS_PRINTFILEDIALOGFAILED); break;
        case Stage::PrepareJob: summary = Resource(unavailable ? IDS_PRINTNOPRINTER : IDS_PRINTFAILED); break;
        case Stage::Page: summary = FormatStringResource(nullptr, IDS_PRINTPAGEFAILED, pageNumber); break;
        case Stage::FinishJob: summary = Resource(IDS_PRINTFINISHFAILED); break;
        case Stage::SaveFile: summary = Resource(IDS_PRINTFILEFAILED); break;
    }
    const auto messageSummary = printer.empty() ? summary : FormatStringResource(nullptr, IDS_PRINTDESTINATIONFAILED, summary, printer);
    const auto error          = ErrorText(result);
    const auto message        = FormatStringResource(nullptr, IDS_PRINTERRORDETAIL, messageSummary, error);
    _feedback->SetText(messageSummary);
    if (_printPreview)
        _printPreview->SetStatus(messageSummary);
    if (! _options.headless)
    {
        const HWND owner = _printPreview ? _printPreview->Window() : _window.get();
        (void)DxUi::Failure::InvokeNative(&MessageBoxW, owner, message.c_str(), Resource(IDS_ERROR).c_str(), static_cast<UINT>(MB_OK | MB_ICONERROR));
    }
}

void Application::RememberPrintImage(const DxUi::RichText::RichImageRequest& request, std::shared_ptr<const DxUi::RichText::RichImage> pixels)
{
    using DxUi::RichText::RichImageSourceKind;
    if (! pixels || (request.SourceKind() != RichImageSourceKind::RemoteHttp && request.SourceKind() != RichImageSourceKind::NetworkFile))
        return;
    constexpr std::size_t limit = 64u * 1024u * 1024u;
    std::erase_if(_printNetworkImages,
                  [&](const auto& image) { return image.request.SourceKind() == request.SourceKind() && image.request.Source() == request.Source(); });
    std::size_t total = pixels->RetainedBytes();
    for (const auto& image : _printNetworkImages)
        total += image.pixels->RetainedBytes();
    while (! _printNetworkImages.empty() && (total > limit || _printNetworkImages.size() >= RichImageLoader::MaximumResults))
    {
        total -= _printNetworkImages.front().pixels->RetainedBytes();
        _printNetworkImages.erase(_printNetworkImages.begin());
    }
    if (total <= limit)
        _printNetworkImages.push_back({request, std::move(pixels)});
}
} // namespace RedTilde
