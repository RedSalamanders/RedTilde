#include "Application.h"
#include "Common/CrashHandler.h"
#include "Common/NativeCalls.h"
#include "PrintPreview.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <psapi.h>
#include <wincodec.h>

namespace RedTilde
{
void Application::Prepare(unsigned turns)
{
    if (IsClosing() || ! _host.GetRoot())
        return;
    const auto cosmeticsPending = [](const DxUi::RichTextEditor* target)
    {
        if (! target || ! target->IsVisible())
            return false;
        const auto state = target->DebugGetState();
        return state.markdownPreparationPending || state.sourceInkPreparationPending || state.codeInkPreparationPending || state.sourceGuidesPreparationPending;
    };
    for (unsigned index = 0; index < turns; ++index)
    {
        if (_bitmapWriter.HasResult())
            ServiceBitmapPaste();
        if (IsClosing() || ! _host.GetRoot())
            return;
        (void)_host.RenderInitialFrameForShow();
        const bool identityRelayout = _identityMeasure.typographyRevision != _host.GetTypographyRevision();
        if (identityRelayout)
            Layout();
        ServiceImages();
        MSG message{};
        unsigned processed{};
        while (processed++ < 64 && PeekMessageW(&message, _window.get(), 0, 0, PM_REMOVE))
        {
            if (! _host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (IsClosing() || ! _host.GetRoot())
            return;
        ReconcileSpellImeExit();
        if (_files.Busy())
            FileCompleted(); // also drains a result if its post met a full queue
        RecoveryCompleted();
        FolderCompleted();
        SpellCompleted();
        if (IsClosing() || ! _host.GetRoot())
            return;
        // A render may publish new immutable syntax. Reconcile dependent chrome
        // before declaring quiescence, then paint it on the next turn. Hidden
        // hosts deliberately do not receive invalidation-driven WM_PAINT.
        const bool refreshed = ReconcilePostPreparePlacement() || identityRelayout;
        // This bounded harness drain also settles optional pixels for captures.
        // Production input/frame readiness remains the host's earlier content
        // boundary; its event loop never calls this synchronous convenience loop.
        if (! refreshed && ! _host.HasPendingContentPreparation() && ! cosmeticsPending(_editor) && ! cosmeticsPending(_mirror) &&
            _editor->GetImageResourceStatistics().pendingSources == 0 && ! _files.Busy() && _recovery.Ready() && ! _spell.immediatePending &&
            ! _spellService.Busy() && (! _folderFiles.IsActive() || _folderSnapshot || FAILED(_folderFiles.Status()) || FAILED(_folderAdmissionStatus)))
            break;
        // Harness-only bounded event wait; production blocks in GetMessage.
        (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}

bool Application::Capture(const std::filesystem::path& path, bool prepareContent)
{
    if (prepareContent && ! _printPreview)
        Prepare();
    DxUi::Button* captureHover{};
    if (_options.headless && path == _options.capture && _options.scenario == L"panel-toggle-hover" && _panelToggle)
    {
        captureHover      = _panelToggle;
        const auto bounds = captureHover->GetBounds();
        const float scale = _host.GetDpi() / 96.f;
        const LONG px     = static_cast<LONG>((bounds.left + bounds.right) * .5f * scale);
        const LONG py     = static_cast<LONG>((bounds.top + bounds.bottom) * .5f * scale);
        bool handled{};
        (void)_host.HandleMessage(_window.get(), WM_MOUSEMOVE, 0, MAKELPARAM(px, py), handled);
        if (! captureHover->IsHovered())
            return false;
    }
    if (_options.headless && path == _options.capture && (_options.scenario == L"caption-close-hover" || _options.scenario == L"caption-maximize-hover"))
    {
        const bool close  = _options.scenario == L"caption-close-hover";
        captureHover      = close ? _closeButton : _maximizeButton;
        const auto bounds = captureHover->GetBounds();
        const auto point  = _host.DipPointToScreenPoint({(bounds.left + bounds.right) * .5f, (bounds.top + bounds.bottom) * .5f});
        bool handled{};
        // Preparation pumps queued leave messages for this hidden window.
        // Apply owned synthetic hover after that pump, immediately before the
        // frame readback, without moving the pointer or taking native input.
        (void)_host.HandleMessage(
            _window.get(), WM_NCMOUSEMOVE, close ? HTCLOSE : HTMAXBUTTON, MAKELPARAM(static_cast<SHORT>(point.x), static_cast<SHORT>(point.y)), handled);
        if (handled || ! captureHover->IsHovered())
            return false;
    }
    DxUi::WindowHostBitmapCapture bitmap;
    // Stage the completed frame before Present rotates the swap-chain buffers.
    // The canonical hidden capture path never shows or activates this HWND.
    auto& captureHost = _printPreview ? _printPreview->Host() : _host;
    if (! captureHost.DebugCaptureBitmap(bitmap, true))
        return false;
    if (captureHover && ! captureHover->IsHovered())
        return false;
    wil::com_ptr<IWICImagingFactory> factory;
    wil::com_ptr<IWICStream> stream;
    wil::com_ptr<IWICBitmapEncoder> encoder;
    wil::com_ptr<IWICBitmapFrameEncode> frame;
    wil::com_ptr<IPropertyBag2> properties;
    if (FAILED(DxUi::Failure::InvokeNative(&CoCreateInstance,
                                           CLSID_WICImagingFactory,
                                           nullptr,
                                           static_cast<DWORD>(CLSCTX_INPROC_SERVER),
                                           __uuidof(IWICImagingFactory),
                                           reinterpret_cast<void**>(factory.put()))))
        return false;
    if (FAILED(InvokeNativeMethod(factory.get(), &IWICImagingFactory::CreateStream, stream.put())) ||
        FAILED(InvokeNativeMethod(stream.get(), &IWICStream::InitializeFromFilename, path.c_str(), static_cast<DWORD>(GENERIC_WRITE))))
        return false;
    if (FAILED(InvokeNativeMethod(factory.get(), &IWICImagingFactory::CreateEncoder, GUID_ContainerFormatPng, nullptr, encoder.put())) ||
        FAILED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::Initialize, stream.get(), WICBitmapEncoderNoCache)))
        return false;
    if (FAILED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::CreateNewFrame, frame.put(), properties.put())) ||
        FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::Initialize, properties.get())) ||
        FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::SetSize, bitmap.widthPx, bitmap.heightPx)))
        return false;
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (FAILED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::SetPixelFormat, &format)) || ! IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA))
        return false;
    if (FAILED(InvokeNativeMethod(frame.get(),
                                  &IWICBitmapFrameEncode::WritePixels,
                                  bitmap.heightPx,
                                  bitmap.widthPx * 4u,
                                  static_cast<UINT>(bitmap.bgraPixels.size()),
                                  bitmap.bgraPixels.data())))
        return false;
    return SUCCEEDED(InvokeNativeMethod(frame.get(), &IWICBitmapFrameEncode::Commit)) &&
           SUCCEEDED(InvokeNativeMethod(encoder.get(), &IWICBitmapEncoder::Commit));
}

int Application::Run()
{
#if defined(ENABLE_TESTS)
    if (! _options.designInteractive.empty())
        return RunDesignInteractiveProbe(*this, _options);
#endif
    if (! _options.performance.empty())
        return RunPerformanceProbe(*this, _options);
#if defined(ENABLE_TESTS)
    if (_options.recoveryProbe)
    {
        // Isolated harness fixtures: exercise the real Files worker, independent
        // histories and checkpoint leases before the intentional child crash.
        const auto directory = _options.dataDirectory / L"Workspace";
        std::filesystem::create_directories(directory);
        const auto alpha = directory / L"Alpha.md", beta = directory / L"Beta.md";
        if (FAILED(WriteBytesAtomic(alpha, "\xef\xbb\xbf# Alpha\r\n")) || FAILED(WriteBytesAtomic(beta, "# Beta\n")))
            return 6;
        StoredDocument loaded;
        if (FAILED(LoadDocument(alpha, loaded)))
            return 6;
        SetDocument(loaded.text, alpha, loaded.bom, loaded.stamp);
        _editor->SetSelection({0, 0});
        (void)_editor->ReplaceSelection(L"Recovered after crash: 東京 😀\r\n");
        OpenPath(beta);
        for (unsigned turn = 0; turn < 1000 && _session.path != beta; ++turn)
        {
            Prepare(32);
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        }
        if (_session.path != beta || _inactiveDocuments.size() != 1)
            return 6;
        _editor->SetSelection({0, 0});
        (void)_editor->ReplaceSelection(L"A second unsaved copy: 世界\n");
        CheckpointWorkspaceRecovery();
        for (unsigned turn = 0; turn < 1000 && ! _recovery.Settled(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        if (! _recovery.Settled() || FAILED(_recovery.Status()))
            return 6;
        CrashProbe();
    }
    if (_options.recoveryRestartProbe)
    {
        Prepare();
        RecoveryCompleted();
        if (_recoveryCandidates.size() != 2)
            return 7;
        Execute(Command::RestoreAllRecovery);
        const auto expected = [&](const std::filesystem::path& path, std::wstring_view text, bool hasBom)
        {
            return path.filename() == (hasBom ? L"Alpha.md" : L"Beta.md") &&
                   text == (hasBom ? L"Recovered after crash: 東京 😀\r\n# Alpha\r\n" : L"A second unsaved copy: 世界\n# Beta\n");
        };
        bool correct = Dirty() && _session.recovered && _inactiveDocuments.size() == 1 && expected(_session.path, _editor->GetText(), _session.bom);
        for (const auto& document : _inactiveDocuments)
            correct =
                correct && document.dirty && document.metadata.recovered && expected(document.metadata.path, document.state.Text(), document.metadata.bom);
        StoredDocument original;
        correct =
            correct && SUCCEEDED(LoadDocument(_options.dataDirectory / L"Workspace/Alpha.md", original)) && original.text == L"# Alpha\r\n" && original.bom;
        correct = correct && SUCCEEDED(LoadDocument(_options.dataDirectory / L"Workspace/Beta.md", original)) && original.text == L"# Beta\n" && ! original.bom;
        CheckpointWorkspaceRecovery();
        _recovery.Stop();
        for (unsigned turn = 0; turn < 1000 && ! _recovery.Finished(); ++turn)
            (void)WaitForSingleObject(GetCurrentProcess(), 5);
        return correct && _recovery.Finished() && SUCCEEDED(_recovery.Status()) ? 0 : 8;
    }
    if (_options.preferencesProbe)
    {
        Execute(Command::ThemeLight);
        Execute(Command::ThemeDark);
        Execute(Command::ThemePaper);
        (void)SendMessageW(_window.get(), WM_CLOSE, 0, 0);
        if (_window)
            (void)SendMessageW(_window.get(), WM_CLOSE, 0, 0);
        MSG message{};
        while (_window && GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            if (! _host.DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        return 0;
    }
#endif
    if (_options.headless)
    {
        _settings.appearance =
            _options.scenario == L"sample-document-dark" || _options.scenario == L"task-presentation-dark" || _options.scenario == L"compose-dark" ||
                    _options.scenario == L"split-dark" || _options.scenario == L"math-dark" || _options.scenario == L"quote-scroll-dark" ||
                    _options.scenario == L"diagram-label-dark" || _options.scenario == L"reference-dark" || _options.scenario == L"markdown-style" ||
                    _options.scenario == L"caption-close-hover" || _options.scenario == L"caption-maximize-hover" ||
                    _options.scenario == L"pane-hover-reveal-dark" || _options.scenario == L"split-unlinked-dark" || _options.scenario == L"spelling-dark"
                ? Appearance::Dark
            : _options.scenario == L"compose-paper" ? Appearance::Paper
                                                    : Appearance::Light;
        if (! _options.capture.empty() && ! _options.scenario.starts_with(L"spelling-"))
        {
#if defined(ENABLE_TESTS)
            if (_options.scenario == L"settings" || _options.scenario == L"settings-about")
            {
                auto deterministicProvider = std::make_shared<FakeSpellProvider>();
                deterministicProvider->SetInstalledLanguages({L"en-US"});
                deterministicProvider->SetLanguageAvailability(L"en-US", ProviderAvailability::Ready);
                _spellService.SetProviderForTest(deterministicProvider);
            }
            else
#endif
            {
                _settings.spelling.enabled = false;
            }
        }
        ApplyTheme();
        if (_options.scenario == L"math" || _options.scenario == L"math-dark")
            SetDocument(L"# Mathematics, in the flow\n\nInline $E=mc^2$ shares the text baseline. Exact TeX stays one F2 away.\n\n"
                        L"$$\nx=\\frac{-b+\\sqrt{b^2-4ac}}{2a}\n$$\n\n"
                        L"$$\nA=\\begin{bmatrix}a&b\\\\c&d\\end{bmatrix}\\quad \\vec{v}=\\begin{pmatrix}x\\\\y\\end{pmatrix}\n$$\n\n"
                        L"$$\n\\begin{aligned}f(x)&=x^2+1\\\\f'(x)&=2x\\end{aligned}\n$$\n\n"
                        L"Unsupported commands keep their exact source: $\\input{private}$.\n");
        if (_options.scenario == L"mermaid")
            SetDocument(L"# Diagrams, in the flow\n\n## Flowchart\n\n```mermaid\nflowchart LR\nDraft[Draft] --> Review[Review]\n"
                        L"Review --> Saved[Saved]\n```\n\n"
                        L"## Sequence diagram\n\n```mermaid\nsequenceDiagram\nparticipant Writer\nparticipant Notebook\n"
                        L"Writer->>Notebook: Save draft\nNotebook-->>Writer: Saved\n```\n\n"
                        L"## State diagram\n\n```mermaid\nstateDiagram-v2\ndirection LR\n[*] --> Draft\nDraft --> Reviewed : review\n"
                        L"Reviewed --> [*]\n```\n");
        if (_options.scenario == L"recovery")
        {
            _recoveryCandidates.push_back({L"0123456789abcdef0123456789abcdef",
                                           {L"# Your writing is here\r\n\r\nThe unsaved paragraph, including 東京 and 😀, is ready to restore.\r\n",
                                            L"C:\\Notes\\My next idea.md",
                                            true,
                                            {0, 0},
                                            12}});
            ShowRecovery();
        }
        if (_options.scenario == L"source")
            SetView(EditorView::Source);
        if (_options.scenario == L"split" || _options.scenario == L"split-dark")
            SetView(EditorView::Split);
        if (_options.scenario == L"settings")
            Execute(Command::Settings);
        Prepare();
        if (! _options.benchmark.empty())
        {
            using Clock               = std::chrono::steady_clock;
            constexpr unsigned frames = 60;
            const auto start          = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
                (void)_host.RenderInitialFrameForShow();
            const double cleanSeconds = std::chrono::duration<double>(Clock::now() - start).count();
            const auto dirtyStart     = Clock::now();
            for (unsigned index = 0; index < frames; ++index)
            {
                _editor->SetSelection({0, 0});
                (void)_editor->ReplaceSelection(L"x");
                (void)_host.RenderInitialFrameForShow();
                (void)_editor->Undo();
                (void)_host.RenderInitialFrameForShow();
            }
            const double dirtySeconds = std::chrono::duration<double>(Clock::now() - dirtyStart).count();
            PROCESS_MEMORY_COUNTERS_EX memory{};
            (void)GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory));
            std::ofstream report(_options.benchmark, std::ios::binary);
            report << "{\n  \"scenario\":\"sample-richtext-native-hidden\",\n  \"cleanFrames\":" << frames
                   << ",\n  \"cleanRenderFps\":" << frames / cleanSeconds << ",\n  \"dirtyFrames\":" << frames * 2
                   << ",\n  \"dirtyEditAndRenderMs\":" << dirtySeconds * 1000 / (frames * 2) << ",\n  \"workingSetBytes\":" << memory.WorkingSetSize
                   << ",\n  \"privateBytes\":" << memory.PrivateUsage << ",\n  \"dpi\":" << _host.GetDpi() << ",\n  \"widthPx\":" << _host.DebugGetWidthPx()
                   << ",\n  \"heightPx\":" << _host.DebugGetHeightPx()
                   << ",\n  \"qualification\":\"initial application baseline only; forced native renders, not presentation FPS or a paired "
                      "non-regression claim\"\n}\n";
            if (! report)
                return 4;
        }
        if (_options.scenario == L"context-selection")
        {
            const auto text  = _editor->GetText();
            const auto begin = text.find(L"Good writing");
            if (begin != text.npos)
            {
                _editor->SetSelection({begin, begin + 12});
                SelectionChanged();
            }
        }
        if (_options.scenario == L"local-source")
        {
            const auto begin = _editor->GetText().find(L"Good writing");
            if (begin != std::wstring::npos)
            {
                _editor->SetSelection({begin, begin + 12});
                Execute(Command::LocalSource);
            }
        }
        if (_options.scenario == L"math" || _options.scenario == L"math-dark")
        {
            const auto begin = _editor->GetText().find(L"\\frac");
            if (begin != std::wstring::npos)
            {
                _editor->SetSelection({begin, begin});
                SelectionChanged();
            }
        }
        PrepareDesignScenario();
        return _options.capture.empty() || Capture(_options.capture) ? 0 : 3;
    }
    if (_startupRestore.IsWaitingForRecovery())
        RecoveryCompleted(); // Also settle a scan that completed before the ordinary message loop started.
    (void)_host.PrimeForShow();
    if (_identityMeasure.typographyRevision != _host.GetTypographyRevision())
    {
        Layout();
        (void)_host.PrimeForShow();
    }
    ShowWindow(_window.get(), InitialShowCommand());
    UpdateWindow(_window.get());
    RecordWindowPlacement(true);
    _host.SetFocusControl(ActiveEditor(), MayRequestNativeFocus());
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        DispatchUiMessage(message);
    }
    return static_cast<int>(message.wParam);
}
} // namespace RedTilde
