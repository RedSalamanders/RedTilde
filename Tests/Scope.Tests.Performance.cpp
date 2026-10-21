#include "RedTilde/Application.h"
#include <DxUi/RichSemanticDocument.h>
#include <algorithm>
#include <chrono>
#include <d2d1_3.h>
#include <dxgi1_6.h>
#include <fstream>
#include <iomanip>
#include <psapi.h>
#include <stdexcept>
#include <tlhelp32.h>

namespace RedTilde
{
namespace
{
using Clock = std::chrono::steady_clock;
void Require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
void Check(HRESULT value, const char *message)
{
    Require(SUCCEEDED(value), message);
}
double Micros(Clock::time_point start)
{
    return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
}
std::uint64_t OwnerCycles()
{
    ULONG64 cycles{};
    Require(QueryThreadCycleTime(GetCurrentThread(), &cycles) != FALSE, "owner CPU cycles unavailable");
    return cycles;
}
void Quote(std::ostream &out, std::wstring_view value)
{
    out << '"';
    for (const auto c : DxUi::RichText::EncodeUtf8(value))
    {
        if (c == '"' || c == '\\')
            out << '\\';
        if (static_cast<unsigned char>(c) < 32)
            out << '?';
        else
            out << c;
    }
    out << '"';
}
void Samples(std::ostream &out, std::string_view name, const std::vector<double> &values)
{
    out << '"' << name << "\":[";
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i)
            out << ',';
        out << values[i];
    }
    out << ']';
}
struct GpuFence
{
    wil::com_ptr_nothrow<IDXGIDevice2> dxgi;
    wil::com_ptr_nothrow<IDXGIAdapter3> adapter3;
    wil::com_ptr_nothrow<ID3D11DeviceContext> immediate;
    wil::unique_event event;
    DXGI_ADAPTER_DESC1 adapter{};
    LARGE_INTEGER driver{};
    explicit GpuFence(DxUi::ControlHost &host)
    {
        auto *context = host.GetDeviceContext();
        Require(context != nullptr, "missing native render context");
        wil::com_ptr_nothrow<ID2D1Device> d2d;
        context->GetDevice(d2d.put());
        wil::com_ptr_nothrow<ID2D1Device2> d2d2;
        Check(d2d.query_to(d2d2.put()), "missing native D2D2 device");
        wil::com_ptr_nothrow<IDXGIDevice> device;
        Check(d2d2->GetDxgiDevice(device.put()), "missing native DXGI device");
        Check(device.query_to(dxgi.put()), "GPU completion events unavailable");
        wil::com_ptr_nothrow<ID3D11Device> d3d;
        Check(device.query_to(d3d.put()), "missing underlying D3D11 device");
        d3d->GetImmediateContext(immediate.put());
        wil::com_ptr_nothrow<IDXGIAdapter> base;
        Check(device->GetAdapter(base.put()), "missing native graphics adapter");
        wil::com_ptr_nothrow<IDXGIAdapter1> description;
        Check(base.query_to(description.put()), "adapter flags unavailable");
        Check(description->GetDesc1(&adapter), "adapter description unavailable");
        Check(base->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver), "graphics driver version unavailable");
        (void)base.query_to(adapter3.put());
        event.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        Require(static_cast<bool>(event), "GPU completion event unavailable");
    }
    void Wait()
    {
        immediate->Flush();
        Check(dxgi->EnqueueSetEvent(event.get()), "GPU completion event submission failed");
        Require(WaitForSingleObject(event.get(), 5000) == WAIT_OBJECT_0, "GPU completion deadline expired");
    }
};
struct Memory
{
    PROCESS_MEMORY_COUNTERS_EX process{};
    DWORD handles{}, gdi{}, user{}, threads{};
    std::uint64_t cpu100ns{};
};
Memory Snapshot()
{
    Memory value;
    value.process.cb = sizeof(value.process);
    Require(GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&value.process), sizeof(value.process)) != FALSE,
            "process memory unavailable");
    Require(GetProcessHandleCount(GetCurrentProcess(), &value.handles) != FALSE, "process handle count unavailable");
    value.gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    value.user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    FILETIME created{}, exited{}, kernel{}, user{};
    Require(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) != FALSE, "process CPU unavailable");
    value.cpu100ns = (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) +
                     (static_cast<std::uint64_t>(user.dwHighDateTime) << 32 | user.dwLowDateTime);
    wil::unique_handle all(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
    Require(static_cast<bool>(all), "thread inventory unavailable");
    THREADENTRY32 entry{sizeof(entry)};
    if (Thread32First(all.get(), &entry))
        do
        {
            if (entry.th32OwnerProcessID == GetCurrentProcessId())
                ++value.threads;
        } while (Thread32Next(all.get(), &entry));
    return value;
}
void MemoryJson(std::ostream &out, const Memory &value)
{
    out << "\"privateBytes\":" << value.process.PrivateUsage << ",\"workingSetBytes\":" << value.process.WorkingSetSize
        << ",\"handles\":" << value.handles << ",\"gdiObjects\":" << value.gdi << ",\"userObjects\":" << value.user
        << ",\"threads\":" << value.threads << ",\"processCpu100ns\":" << value.cpu100ns;
}
} // namespace
class PerformanceProbe
{
    Application &app;
    const LaunchOptions &options;
    std::unique_ptr<GpuFence> gpu;
    unsigned Pump()
    {
        MSG message{};
        unsigned count{};
        while (count < 256 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            ++count;
            Require(message.message != WM_QUIT, "unexpected process quit during native probe");
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (app.closing)
        {
            app.FinishClose();
            return count;
        }
        app.ServiceImages();
        if (app.files.Busy())
            app.FileCompleted();
        app.RecoveryCompleted();
        return count;
    }
    void Ready(std::wstring_view expected)
    {
        const auto deadline = GetTickCount64() + 5000;
        do
        {
            Require(app.host.RenderInitialFrameForShow(), "native frame submission failed");
            const auto processed = Pump();
            const auto semantic = app.editor->GetAcknowledgedSemanticDocument(app.host);
            const auto mirror = app.mirror->DebugGetState();
            if (!app.host.HasPendingContentPreparation() && semantic && semantic->Syntax()->Source() == expected &&
                app.editor->GetImageResourceStatistics().pendingSources == 0 &&
                (app.settings.view != EditorView::Split || (!mirror.preparationPending && mirror.layoutReady)) && !app.files.Busy() &&
                app.recovery.Ready())
            {
                Require(app.host.DebugGetPresentFailureCount() == 0, "native presentation reported a failed frame");
                return;
            }
            if (!processed && app.editor->GetImageResourceStatistics().pendingSources)
                (void)MsgWaitForMultipleObjectsEx(0, nullptr, 5, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        } while (GetTickCount64() < deadline);
        throw std::runtime_error("current native semantic frame missed its preparation deadline");
    }
    void Idle(DWORD milliseconds)
    {
        const auto deadline = GetTickCount64() + milliseconds;
        do
        {
            Pump();
            const auto now = GetTickCount64();
            if (now >= deadline)
                break;
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, static_cast<DWORD>(std::min<ULONGLONG>(deadline - now, 50)), QS_ALLINPUT,
                                              MWMO_INPUTAVAILABLE);
        } while (true);
    }
    void Resource(std::ostream &out, std::string_view name)
    {
        out << "{\"phase\":\"" << name << "\",";
        MemoryJson(out, Snapshot());
        const auto images = app.editor->GetImageResourceStatistics();
        out << ",\"generation\":" << app.host.GetResourceGeneration() << ",\"renders\":" << app.host.DebugGetRenderCount()
            << ",\"brushes\":" << app.host.DebugGetBrushCacheSize() << ",\"formats\":" << app.host.DebugGetConfiguredTextFormatCount()
            << ",\"imageDecodedBytes\":" << images.decodedBytes << ",\"imagePending\":" << images.pendingSources;
        DXGI_QUERY_VIDEO_MEMORY_INFO memory{};
        const HRESULT hr = gpu->adapter3 ? gpu->adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memory) : E_NOINTERFACE;
        out << ",\"dxgiLocalUsageBytes\":";
        if (SUCCEEDED(hr))
            out << memory.CurrentUsage;
        else
            out << "null";
        out << ",\"dxgiUsageHresult\":" << static_cast<std::uint32_t>(hr) << '}';
        Require(app.host.DebugGetBrushCacheSize() <= DxUi::ControlHost::kSolidBrushCacheLimit, "native brush cache exceeded its bound");
        Require(app.host.DebugGetConfiguredTextFormatCount() <= DxUi::ControlHost::kConfiguredTextFormatCacheLimit,
                "native format cache exceeded its bound");
        Require(images.decodedBytes <= 128ull * DxUi::RichText::RichImage::MaximumPixels * 4, "native decoded images exceeded their bound");
    }

  public:
    PerformanceProbe(Application &value, const LaunchOptions &launch) : app(value), options(launch) {}
    int Run()
    {
        app.settings.appearance = Appearance::Light;
        app.ApplyTheme();
        app.SetView(options.scenario == L"split" ? EditorView::Split : EditorView::Compose);
        const auto source = app.editor->GetText();
        const auto at = source.find(L"S5_EDIT_HERE");
        Require(at != source.npos, "paired fixture is missing its fixed prose edit site");
        auto edited = source;
        edited.insert(at, 1, L'x');
        Ready(source);
        gpu = std::make_unique<GpuFence>(app.host);
        Require(!options.forceWarp || (gpu->adapter.Flags & DXGI_ADAPTER_FLAG_SOFTWARE), "explicit WARP did not create a software adapter");
        gpu->Wait();
        std::vector<double> command, preparation, completion, total, clean, cpu;
        for (auto *values : {&command, &preparation, &completion, &total, &clean, &cpu})
            values->reserve(options.performanceCycles * 2);
        const auto activity = app.activity;
        const auto initial = Snapshot();
        for (unsigned cycle = 0; cycle < options.performanceCycles + 2; ++cycle)
        {
            app.editor->SetSelection({at, at});
            for (unsigned undo = 0; undo < 2; ++undo)
            {
                const auto start = Clock::now();
                const auto cpuStart = OwnerCycles();
                Require(undo ? app.editor->Undo() : app.editor->ReplaceSelection(L"x"), "paired edit/undo was not admitted");
                const double commandUs = Micros(start);
                const auto prepare = Clock::now();
                Ready(undo ? std::wstring_view(source) : std::wstring_view(edited));
                const double prepareUs = Micros(prepare);
                const auto finish = Clock::now();
                gpu->Wait();
                const double gpuUs = Micros(finish), totalUs = Micros(start);
                const auto cpuCycles = OwnerCycles() - cpuStart;
                if (cycle >= 2)
                {
                    command.push_back(commandUs);
                    preparation.push_back(prepareUs);
                    completion.push_back(gpuUs);
                    total.push_back(totalUs);
                    cpu.push_back(static_cast<double>(cpuCycles));
                }
            }
            Require(app.editor->GetText() == source && app.editor->GetSelection() == DxUi::RichText::Selection{at, at},
                    "paired undo changed source or selection");
            const auto start = Clock::now();
            Require(app.host.RenderInitialFrameForShow(), "clean native frame failed");
            gpu->Wait();
            if (cycle >= 2)
                clean.push_back(Micros(start));
        }
        const auto final = Snapshot();
        std::ofstream out(options.performance, std::ios::binary);
        out << std::setprecision(12) << "{\"schemaVersion\":1,\"fixture\":\"redtilde-completed-native-v1\",\"view\":\""
            << (app.settings.view == EditorView::Split ? "Split" : "Compose") << "\",\"renderer\":\""
            << ((gpu->adapter.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? "WARP" : "Hardware") << "\",\"adapter\":";
        Quote(out, gpu->adapter.Description);
        out << ",\"vendorId\":" << gpu->adapter.VendorId << ",\"deviceId\":" << gpu->adapter.DeviceId << ",\"driverVersion\":" << gpu->driver.QuadPart
            << ",\"dpi\":" << app.host.GetDpi() << ",\"widthPx\":" << app.host.DebugGetWidthPx() << ",\"heightPx\":" << app.host.DebugGetHeightPx()
            << ",\"sourceUnits\":" << source.size() << ",\"cycles\":" << options.performanceCycles << ",\"samples\": {";
        Samples(out, "commandUs", command);
        out << ',';
        Samples(out, "preparationSubmissionUs", preparation);
        out << ',';
        Samples(out, "gpuWaitUs", completion);
        out << ',';
        Samples(out, "completedEditUs", total);
        out << ',';
        Samples(out, "cleanCompletedUs", clean);
        out << ',';
        Samples(out, "ownerCpuCycles", cpu);
        out << "},\"initial\":{";
        MemoryJson(out, initial);
        out << "},\"final\":{";
        MemoryJson(out, final);
        out << "},\"activity\":{\"changes\":" << app.activity.changes - activity.changes << ",\"layouts\":" << app.activity.layouts - activity.layouts
            << ",\"mirrorResets\":" << app.activity.mirrorResets - activity.mirrorResets
            << ",\"recoverySubmissions\":" << app.activity.recoverySubmissions - activity.recoverySubmissions << "},\"resources\":[";
        if (options.resourceProbe)
        {
            const auto image = options.dataDirectory / L"resource-input.png";
            Require(app.Capture(image), "native resource image fixture capture failed");
            const auto resourceSource = source + L"\n\n![Native resource fixture](<" + image.generic_wstring() + L">)\n";
            app.SetDocument(resourceSource, app.documentPath, app.bom);
            app.host.SetFocusControl(nullptr, false);
            Ready(resourceSource);
            gpu->Wait();
            Require(app.editor->GetImageResourceStatistics().readySources == 1 && app.editor->GetImageResourceStatistics().decodedBytes > 0,
                    "native resource scene did not decode its local image");
            Resource(out, "clean-after-dirty");
            const auto renders = app.host.DebugGetRenderCount(), advances = app.editor->DebugGetState().preparationAdvances;
            Idle(500);
            Require(app.host.DebugGetRenderCount() == renders && app.editor->DebugGetState().preparationAdvances == advances,
                    "clean hidden idle performed unrequested rendering or preparation");
            out << ',';
            Resource(out, "clean-hidden-idle");
            (void)SendMessageW(app.window.get(), WM_SHOWWINDOW, FALSE, 0);
            Pump();
            const auto hidden = app.host.DebugGetRenderCount();
            Idle(500);
            Require(!IsWindowVisible(app.window.get()) && app.host.DebugGetRenderCount() == hidden, "hidden notification did not quiesce rendering");
            out << ',';
            Resource(out, "hidden-notification-idle");
            (void)SendMessageW(app.window.get(), WM_SHOWWINDOW, TRUE, 0);
            Ready(resourceSource);
            gpu->Wait();
            out << ',';
            Resource(out, "resumed-forced-frame");
            RECT outer{};
            Require(GetWindowRect(app.window.get(), &outer) != FALSE, "native resource window bounds unavailable");
            Require(SetWindowPos(app.window.get(), nullptr, 0, 0, outer.right - outer.left - 120, outer.bottom - outer.top - 80,
                                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE) != FALSE,
                    "native resource resize failed");
            Ready(resourceSource);
            gpu->Wait();
            out << ',';
            Resource(out, "resized-forced-frame");
            Require(SetWindowPos(app.window.get(), nullptr, 0, 0, outer.right - outer.left, outer.bottom - outer.top,
                                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE) != FALSE,
                    "native resource size restore failed");
            Ready(resourceSource);
            gpu->Wait();
            const auto before = options.dataDirectory / L"resource-before.png";
            Require(app.Capture(before), "pre-replacement native capture failed");
            std::string pixels;
            Check(ReadBytes(before, pixels), "pre-replacement capture unavailable");
            for (unsigned cycle = 0; cycle < 3; ++cycle)
            {
                const auto generation = app.host.GetResourceGeneration();
                gpu.reset();
                app.host.DebugSimulateDeviceLoss();
                Ready(resourceSource);
                gpu = std::make_unique<GpuFence>(app.host);
                gpu->Wait();
                Require(app.host.GetResourceGeneration() != generation, "device replacement did not advance generation");
                const auto after = options.dataDirectory / (L"resource-after-" + std::to_wstring(cycle) + L".png");
                Require(app.Capture(after), "post-replacement native capture failed");
                std::string actual;
                Check(ReadBytes(after, actual), "post-replacement capture unavailable");
                Require(actual == pixels, "device replacement changed exact native pixels");
                out << ',';
                Resource(out, "recreated-forced-frame");
            }
            Require(app.editor->GetText() == resourceSource, "resource lifecycle changed source");
            const auto hosts = DxUi::DebugGetAttachedWindowHostCount();
            gpu.reset();
            (void)SendMessageW(app.window.get(), WM_CLOSE, 0, 0);
            const auto closeDeadline = GetTickCount64() + 5000;
            while (app.window && GetTickCount64() < closeDeadline)
                Idle(10);
            Require(!app.window && DxUi::DebugGetAttachedWindowHostCount() + 1 == hosts, "native resource close did not retire its host and window");
            out << ",{\"phase\":\"closed\",";
            MemoryJson(out, Snapshot());
            out << ",\"nativeHosts\":" << DxUi::DebugGetAttachedWindowHostCount() << '}';
        }
        out << "],\"completed\":true,\"qualification\":\"hidden native completed frames; physical presentation and input unverified\"}\n";
        Require(static_cast<bool>(out), "native probe report write failed");
        return 0;
    }
};
int RunPerformanceProbe(Application &app, const LaunchOptions &options)
{
    return PerformanceProbe(app, options).Run();
}
} // namespace RedTilde
