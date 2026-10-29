#include "RedTilde/Application.h"
#include "Application.Tests.HeapDiagnostic.h"
#include <DxUi/FrameRuntime.h>
#include <DxUi/RichSemanticDocument.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <d2d1_3.h>
#include <dxgi1_6.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <malloc.h>
#include <optional>
#include <psapi.h>
#include <span>
#include <stdexcept>
#include <tlhelp32.h>
#include <vector>

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
unsigned OwnerProcessor()
{
    PROCESSOR_NUMBER processor{};
    GetCurrentProcessorNumberEx(&processor);
    return static_cast<unsigned>(processor.Group) * 64 + processor.Number;
}
std::uint64_t ProcessCpu100ns()
{
    FILETIME created{}, exited{}, kernel{}, user{};
    Require(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) != FALSE, "process CPU unavailable");
    return (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) +
           (static_cast<std::uint64_t>(user.dwHighDateTime) << 32 | user.dwLowDateTime);
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
constexpr std::size_t FrameMetricCapacity = 64;
// Three independently rounded microsecond stages may exceed their whole-frame duration by at most 3 us.
constexpr std::uint64_t FrameMetricRoundingToleranceUs = 3;
struct FrameMetricRecord
{
    std::uint64_t totalUs{}, updateUs{}, renderUs{}, presentUs{};
};
struct FrameMetricBatch
{
    std::array<FrameMetricRecord, FrameMetricCapacity> frames{};
    std::size_t frameCount{}, hostRenderCount{}, requestedAttempts{}, nextMetric{};
    bool valid{true};
    void Record(std::wstring_view metric, std::uint64_t value) noexcept
    {
        unsigned index{};
        if (metric == L"dxui.frame.total_us")
            index = 0;
        else if (metric == L"dxui.frame.update_us")
            index = 1;
        else if (metric == L"dxui.frame.render_us")
            index = 2;
        else if (metric == L"dxui.frame.present_us")
            index = 3;
        else
            return;
        if (!valid)
            return;
        if (index != nextMetric || (index == 0 && frameCount == frames.size()))
        {
            valid = false;
            return;
        }
        auto &frame = frames[frameCount];
        switch (index)
        {
        case 0:
            frame.totalUs = value;
            break;
        case 1:
            frame.updateUs = value;
            break;
        case 2:
            frame.renderUs = value;
            break;
        case 3:
            frame.presentUs = value;
            ++frameCount;
            break;
        default:
            valid = false;
            return;
        }
        nextMetric = (index + 1) % 4;
    }
    bool Complete() const noexcept
    {
        if (!valid || nextMetric != 0 || frameCount == 0 || frameCount != requestedAttempts || hostRenderCount == 0 || hostRenderCount > frameCount)
            return false;
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto &frame = frames[index];
            if (frame.updateUs > maximum - frame.renderUs)
                return false;
            const auto updateAndRenderUs = frame.updateUs + frame.renderUs;
            if (updateAndRenderUs > maximum - frame.presentUs)
                return false;
            const auto stageTotalUs = updateAndRenderUs + frame.presentUs;
            if (stageTotalUs > frame.totalUs && stageTotalUs - frame.totalUs > FrameMetricRoundingToleranceUs)
                return false;
        }
        return true;
    }
    static void Sink(void *context, std::wstring_view metric, std::uint64_t value) noexcept
    {
        static_cast<FrameMetricBatch *>(context)->Record(metric, value);
    }
};
struct EditorPreparationAttempt
{
    std::size_t frameMetricAttemptIndex{};
    std::uint64_t hostRenderCountBefore{}, hostRenderCountAfter{};
    DxUi::RichTextEditorDebugState primaryBefore{}, primaryAfter{}, mirrorBefore{}, mirrorAfter{};
};
struct EditorPreparationTransaction
{
    std::size_t firstAttempt{}, attemptCount{};
};
class EditorPreparationDiagnostics
{
    std::vector<EditorPreparationAttempt> attempts_;
    std::vector<EditorPreparationTransaction> transactions_;
    bool valid_{true};
    std::size_t currentStart_{}, currentAttempts_{};
    bool transactionActive_{};

    static bool CountersDoNotDecrease(const DxUi::RichTextEditorDebugState &before, const DxUi::RichTextEditorDebugState &after) noexcept
    {
        return before.preparationAdvances <= after.preparationAdvances && before.cancelledPreparations <= after.cancelledPreparations &&
               before.markdownPreparationAdvances <= after.markdownPreparationAdvances &&
               before.semanticPreparationAdvances <= after.semanticPreparationAdvances;
    }

  public:
    static constexpr std::size_t MaximumAttemptsPerTransaction = FrameMetricCapacity;

    void Reserve(std::size_t maximumTransactions)
    {
        Require(maximumTransactions <= std::numeric_limits<std::size_t>::max() / MaximumAttemptsPerTransaction,
                "editor preparation diagnostic capacity overflowed");
        attempts_.reserve(maximumTransactions * MaximumAttemptsPerTransaction);
        transactions_.reserve(maximumTransactions);
        Require(attempts_.capacity() >= maximumTransactions * MaximumAttemptsPerTransaction && transactions_.capacity() >= maximumTransactions,
                "editor preparation diagnostic storage was not reserved before the measured transactions");
    }
    void BeginTransaction() noexcept
    {
        if (transactionActive_ || transactions_.size() == transactions_.capacity())
        {
            valid_ = false;
            return;
        }
        currentStart_ = attempts_.size();
        currentAttempts_ = 0u;
        transactionActive_ = true;
    }
    void Record(const EditorPreparationAttempt &attempt) noexcept
    {
        if (!transactionActive_ || attempts_.size() == attempts_.capacity() || currentAttempts_ == MaximumAttemptsPerTransaction)
        {
            valid_ = false;
            return;
        }
        attempts_.push_back(attempt);
        ++currentAttempts_;
    }
    void EndTransaction(std::size_t requestedAttempts) noexcept
    {
        if (!transactionActive_ || transactions_.size() == transactions_.capacity() || currentAttempts_ == 0u ||
            currentAttempts_ != requestedAttempts || currentAttempts_ > MaximumAttemptsPerTransaction)
        {
            valid_ = false;
            transactionActive_ = false;
            return;
        }
        transactions_.push_back({currentStart_, currentAttempts_});
        transactionActive_ = false;
    }
    std::size_t ReservedBytes() const noexcept
    {
        return attempts_.capacity() * sizeof(EditorPreparationAttempt) + transactions_.capacity() * sizeof(EditorPreparationTransaction);
    }
    std::size_t AttemptCapacity() const noexcept
    {
        return attempts_.capacity();
    }
    std::size_t TransactionCapacity() const noexcept
    {
        return transactions_.capacity();
    }
    std::size_t TransactionCount() const noexcept
    {
        return transactions_.size();
    }
    static constexpr std::size_t AttemptRecordBytes() noexcept
    {
        return sizeof(EditorPreparationAttempt);
    }
    static constexpr std::size_t TransactionRecordBytes() noexcept
    {
        return sizeof(EditorPreparationTransaction);
    }
    bool Complete(std::span<const FrameMetricBatch> frameMetrics, std::size_t expectedTransactions) const noexcept
    {
        if (!valid_ || transactionActive_ || transactions_.size() != expectedTransactions || frameMetrics.size() != expectedTransactions ||
            attempts_.size() > attempts_.capacity())
            return false;
        std::size_t nextAttempt{};
        for (std::size_t transactionIndex = 0; transactionIndex < transactions_.size(); ++transactionIndex)
        {
            const auto &transaction = transactions_[transactionIndex];
            const auto &frames = frameMetrics[transactionIndex];
            if (!frames.Complete() || transaction.firstAttempt != nextAttempt || transaction.attemptCount != frames.frameCount ||
                transaction.attemptCount != frames.requestedAttempts || transaction.attemptCount > MaximumAttemptsPerTransaction ||
                transaction.firstAttempt > attempts_.size() || transaction.attemptCount > attempts_.size() - transaction.firstAttempt)
                return false;
            std::uint64_t transactionRenderDelta{};
            for (std::size_t attemptIndex = 0; attemptIndex < transaction.attemptCount; ++attemptIndex)
            {
                const auto &attempt = attempts_[transaction.firstAttempt + attemptIndex];
                if (attempt.frameMetricAttemptIndex != attemptIndex || attempt.hostRenderCountAfter < attempt.hostRenderCountBefore ||
                    attempt.hostRenderCountAfter - attempt.hostRenderCountBefore > 1u ||
                    !CountersDoNotDecrease(attempt.primaryBefore, attempt.primaryAfter) ||
                    !CountersDoNotDecrease(attempt.mirrorBefore, attempt.mirrorAfter))
                    return false;
                if (attemptIndex != 0u)
                {
                    const auto &previous = attempts_[transaction.firstAttempt + attemptIndex - 1u];
                    if (attempt.hostRenderCountBefore != previous.hostRenderCountAfter ||
                        !CountersDoNotDecrease(previous.primaryAfter, attempt.primaryBefore) ||
                        !CountersDoNotDecrease(previous.mirrorAfter, attempt.mirrorBefore))
                        return false;
                }
                transactionRenderDelta += attempt.hostRenderCountAfter - attempt.hostRenderCountBefore;
            }
            if (transactionRenderDelta != frames.hostRenderCount)
                return false;
            nextAttempt += transaction.attemptCount;
        }
        return nextAttempt == attempts_.size();
    }
    const EditorPreparationAttempt &Attempt(std::size_t index) const noexcept
    {
        return attempts_[index];
    }
    const EditorPreparationTransaction &Transaction(std::size_t index) const noexcept
    {
        return transactions_[index];
    }
};
bool EditorPreparationDiagnosticsOracle()
{
    std::array<FrameMetricBatch, 1> frames{};
    auto &frame = frames.front();
    constexpr std::array<std::array<std::uint64_t, 4>, 2> frameValues{{{{12, 3, 4, 4}}, {{12, 3, 4, 4}}}};
    constexpr std::array<std::wstring_view, 4> frameNames{L"dxui.frame.total_us", L"dxui.frame.update_us", L"dxui.frame.render_us",
                                                          L"dxui.frame.present_us"};
    for (const auto &values : frameValues)
        for (std::size_t metric = 0; metric < frameNames.size(); ++metric)
            frame.Record(frameNames[metric], values[metric]);
    frame.hostRenderCount = 1;
    frame.requestedAttempts = 2;

    const auto makeDiagnostics = [](unsigned mutation)
    {
        EditorPreparationDiagnostics diagnostics;
        diagnostics.Reserve(1u);
        diagnostics.BeginTransaction();
        EditorPreparationAttempt first{};
        first.frameMetricAttemptIndex = 0u;
        first.hostRenderCountBefore = 4u;
        first.hostRenderCountAfter = 4u;
        first.primaryBefore.preparationPending = true;
        first.primaryBefore.layoutGeneration = 1u;
        first.primaryBefore.preparationAdvances = 1u;
        first.primaryBefore.markdownPreparationPending = true;
        first.primaryBefore.markdownPreparationAdvances = 1u;
        first.primaryBefore.lastAdvanceMarkdownWork = 8u;
        first.primaryAfter.preparationPending = true;
        first.primaryAfter.preparationAdvances = 2u;
        first.primaryAfter.markdownPreparationPending = false;
        first.primaryAfter.markdownPreparationAdvances = 2u;
        first.primaryAfter.lastAdvanceMarkdownWork = 9u;
        first.mirrorBefore.preparationPending = true;
        first.mirrorBefore.sourceInkPreparationPending = true;
        first.mirrorBefore.preparationAdvances = 1u;
        first.mirrorBefore.lastAdvanceLayouts = 2u;
        first.mirrorAfter.preparationPending = true;
        first.mirrorAfter.sourceInkPreparationPending = true;
        first.mirrorAfter.preparationAdvances = 2u;
        first.mirrorAfter.lastAdvanceLayouts = 3u;
        diagnostics.Record(first);

        EditorPreparationAttempt second{};
        second.frameMetricAttemptIndex = mutation == 1u ? 0u : 1u;
        second.hostRenderCountBefore = mutation == 6u ? 5u : 4u;
        second.hostRenderCountAfter = mutation == 3u ? 6u : (mutation == 5u ? 4u : 5u);
        second.primaryBefore = first.primaryAfter;
        second.primaryAfter = second.primaryBefore;
        second.primaryAfter.preparationPending = false;
        second.primaryAfter.layoutReady = true;
        second.primaryAfter.layoutGeneration = 2u;
        if (mutation == 2u)
            second.primaryBefore.preparationAdvances = 0u;
        second.mirrorBefore = first.mirrorAfter;
        second.mirrorAfter = second.mirrorBefore;
        second.mirrorAfter.preparationPending = false;
        second.mirrorAfter.sourceInkPreparationPending = false;
        second.mirrorAfter.layoutReady = true;
        second.mirrorAfter.layoutGeneration = 2u;
        second.mirrorAfter.sourceInkRangeCount = 5u;
        diagnostics.Record(second);
        diagnostics.EndTransaction(mutation == 4u ? 1u : 2u);
        return diagnostics;
    };

    const auto valid = makeDiagnostics(0u);
    if (!valid.Complete(frames, 1u) || valid.Attempt(0u).primaryAfter.lastAdvanceMarkdownWork != 9u ||
        valid.Attempt(1u).mirrorAfter.sourceInkRangeCount != 5u)
        return false;
    if (makeDiagnostics(1u).Complete(frames, 1u) || makeDiagnostics(2u).Complete(frames, 1u) || makeDiagnostics(3u).Complete(frames, 1u) ||
        makeDiagnostics(4u).Complete(frames, 1u) || makeDiagnostics(5u).Complete(frames, 1u) || makeDiagnostics(6u).Complete(frames, 1u))
        return false;
    auto mismatchedFrames = frames;
    mismatchedFrames.front().requestedAttempts = 1u;
    if (valid.Complete(mismatchedFrames, 1u) || valid.Complete(frames, 2u))
        return false;
    return true;
}
class FrameMetricSinkScope
{
    bool active{};

  public:
    explicit FrameMetricSinkScope(FrameMetricBatch *batch) noexcept : active(batch != nullptr)
    {
        if (active)
            DxUi::SetFrameMetricSink(&FrameMetricBatch::Sink, batch);
    }
    FrameMetricSinkScope(const FrameMetricSinkScope &) = delete;
    FrameMetricSinkScope &operator=(const FrameMetricSinkScope &) = delete;
    void Reset() noexcept
    {
        if (active)
        {
            DxUi::SetFrameMetricSink(nullptr, nullptr);
            active = false;
        }
    }
    ~FrameMetricSinkScope()
    {
        Reset();
    }
};
bool FrameMetricCollectorOracle() noexcept
{
    constexpr std::array<std::wstring_view, 4> events{L"dxui.frame.total_us", L"dxui.frame.update_us", L"dxui.frame.render_us",
                                                      L"dxui.frame.present_us"};
    FrameMetricBatch empty;
    if (empty.Complete() || !empty.valid || empty.frameCount != 0)
        return false;
    FrameMetricBatch full;
    full.Record(L"dxui.frame.future_metric", 99);
    constexpr std::array<std::array<std::uint64_t, 4>, 2> validValues{{{{10, 2, 3, 4}}, {{24, 5, 6, 7}}}};
    for (const auto &frame : validValues)
        for (std::size_t metric = 0; metric < events.size(); ++metric)
            full.Record(events[metric], frame[metric]);
    full.hostRenderCount = 2;
    full.requestedAttempts = 2;
    if (!full.Complete() || full.frameCount != 2 || full.frames[0].totalUs != 10 || full.frames[0].updateUs != 2 || full.frames[0].renderUs != 3 ||
        full.frames[0].presentUs != 4 || full.frames[1].totalUs != 24 || full.frames[1].updateUs != 5 || full.frames[1].renderUs != 6 ||
        full.frames[1].presentUs != 7)
        return false;
    full.hostRenderCount = 3;
    if (full.Complete())
        return false;
    full.hostRenderCount = 2;
    // Update preparation can return before the host's render counter advances. Keep both attempts.
    full.hostRenderCount = 1;
    if (!full.Complete())
        return false;
    full.requestedAttempts = 3;
    if (full.Complete())
        return false;
    full.requestedAttempts = 2;
    FrameMetricBatch overflow;
    overflow.Record(events[0], std::numeric_limits<std::uint64_t>::max());
    overflow.Record(events[1], std::numeric_limits<std::uint64_t>::max());
    overflow.Record(events[2], 1);
    overflow.Record(events[3], 0);
    overflow.hostRenderCount = 1;
    overflow.requestedAttempts = 1;
    if (overflow.Complete())
        return false;
    FrameMetricBatch rounding;
    rounding.Record(events[0], 6);
    rounding.Record(events[1], 2);
    rounding.Record(events[2], 3);
    rounding.Record(events[3], 4);
    rounding.hostRenderCount = 1;
    rounding.requestedAttempts = 1;
    if (!rounding.Complete())
        return false;
    FrameMetricBatch excess;
    excess.Record(events[0], 5);
    excess.Record(events[1], 2);
    excess.Record(events[2], 3);
    excess.Record(events[3], 4);
    excess.hostRenderCount = 1;
    excess.requestedAttempts = 1;
    if (excess.Complete())
        return false;
    FrameMetricBatch malformed;
    malformed.Record(events[1], 1);
    if (malformed.valid || malformed.Complete())
        return false;
    FrameMetricBatch partial;
    partial.Record(events[0], 1);
    if (partial.Complete() || !partial.valid)
        return false;
    FrameMetricBatch capacity;
    constexpr std::array<std::uint64_t, 4> capacityFrame{10, 2, 3, 4};
    for (std::size_t frame = 0; frame < FrameMetricCapacity; ++frame)
        for (std::size_t metric = 0; metric < events.size(); ++metric)
            capacity.Record(events[metric], capacityFrame[metric]);
    capacity.hostRenderCount = FrameMetricCapacity;
    capacity.requestedAttempts = FrameMetricCapacity;
    if (!capacity.Complete() || capacity.frameCount != FrameMetricCapacity)
        return false;
    capacity.Record(events[0], 1);
    return capacity.frameCount == FrameMetricCapacity && !capacity.valid && !capacity.Complete();
}
void WriteFrameMetricTransactions(std::ostream &out, std::string_view name, const std::vector<FrameMetricBatch> &batches)
{
    out << '"' << name << "\":[";
    for (std::size_t i = 0; i < batches.size(); ++i)
    {
        if (i)
            out << ',';
        const auto &batch = batches[i];
        out << "{\"complete\":" << (batch.Complete() ? "true" : "false") << ",\"attemptCount\":" << batch.frameCount
            << ",\"requestedAttemptCount\":" << batch.requestedAttempts << ",\"hostRenderCount\":" << batch.hostRenderCount
            << ",\"attemptCountMinusHostRenderCount\":" << batch.frameCount - batch.hostRenderCount << ",\"attempts\":[";
        for (std::size_t frameIndex = 0; frameIndex < batch.frameCount; ++frameIndex)
        {
            if (frameIndex)
                out << ',';
            const auto &frame = batch.frames[frameIndex];
            out << "{\"totalUs\":" << frame.totalUs << ",\"updateUs\":" << frame.updateUs << ",\"renderUs\":" << frame.renderUs
                << ",\"presentUs\":" << frame.presentUs << '}';
        }
        out << "]}";
    }
    out << ']';
}
void WriteEditorPreparationState(std::ostream &out, const DxUi::RichTextEditorDebugState &state)
{
    out << "{\"layoutReady\":" << (state.layoutReady ? "true" : "false") << ",\"layoutGeneration\":" << state.layoutGeneration
        << ",\"retainedGeometryChunks\":" << state.retainedGeometryChunks << ",\"preparedGeometryChunks\":" << state.preparedGeometryChunks
        << ",\"reusedGeometryChunks\":" << state.reusedGeometryChunks << ",\"preparedSourceUnits\":" << state.preparedSourceUnits
        << ",\"wholeGeometryFallback\":" << (state.wholeGeometryFallback ? "true" : "false")
        << ",\"wholeRasterFallback\":" << (state.wholeRasterFallback ? "true" : "false") << ",\"wholeLayoutsPrepared\":" << state.wholeLayoutsPrepared
        << ",\"wholeLayoutSourceUnits\":" << state.wholeLayoutSourceUnits
        << ",\"preparationPending\":" << (state.preparationPending ? "true" : "false") << ",\"preparationAdvances\":" << state.preparationAdvances
        << ",\"cancelledPreparations\":" << state.cancelledPreparations << ",\"lastAdvanceLayouts\":" << state.lastAdvanceLayouts
        << ",\"lastAdvanceBlockSteps\":" << state.lastAdvanceBlockSteps
        << ",\"markdownPreparationPending\":" << (state.markdownPreparationPending ? "true" : "false")
        << ",\"markdownPreparationAdvances\":" << state.markdownPreparationAdvances
        << ",\"lastAdvanceMarkdownWork\":" << state.lastAdvanceMarkdownWork
        << ",\"semanticPreparationPending\":" << (state.semanticPreparationPending ? "true" : "false")
        << ",\"semanticPreparationAdvances\":" << state.semanticPreparationAdvances
        << ",\"lastAdvanceSemanticWork\":" << state.lastAdvanceSemanticWork
        << ",\"sourceInkPreparationPending\":" << (state.sourceInkPreparationPending ? "true" : "false")
        << ",\"sourceInkRangeCount\":" << state.sourceInkRangeCount << '}';
}
void WriteEditorPreparationDiagnostics(std::ostream &out, const EditorPreparationDiagnostics &diagnostics)
{
    out << "{\"protocol\":\"app-editor-preparation-v1\",\"complete\":true,\"snapshotSource\":\"DxUi.RichTextEditor.DebugGetState\","
           "\"attribution\":\"state deltas only; no per-stage CPU attribution\",\"storage\":{\"reservedBytes\":"
        << diagnostics.ReservedBytes() << ",\"attemptCapacity\":" << diagnostics.AttemptCapacity()
        << ",\"transactionCapacity\":" << diagnostics.TransactionCapacity()
        << ",\"attemptRecordBytes\":" << EditorPreparationDiagnostics::AttemptRecordBytes()
        << ",\"transactionRecordBytes\":" << EditorPreparationDiagnostics::TransactionRecordBytes()
        << ",\"maxAttemptsPerTransaction\":" << EditorPreparationDiagnostics::MaximumAttemptsPerTransaction
        << ",\"retainedThroughPostMeasurement\":true},\"transactions\":[";
    for (std::size_t transactionIndex = 0; transactionIndex < diagnostics.TransactionCount(); ++transactionIndex)
    {
        if (transactionIndex)
            out << ',';
        const auto &transaction = diagnostics.Transaction(transactionIndex);
        out << "{\"complete\":true,\"firstAttemptIndex\":" << transaction.firstAttempt << ",\"attemptCount\":" << transaction.attemptCount
            << ",\"attempts\":[";
        for (std::size_t attemptIndex = 0; attemptIndex < transaction.attemptCount; ++attemptIndex)
        {
            if (attemptIndex)
                out << ',';
            const auto &attempt = diagnostics.Attempt(transaction.firstAttempt + attemptIndex);
            out << "{\"frameMetricAttemptIndex\":" << attempt.frameMetricAttemptIndex
                << ",\"hostRenderCountBefore\":" << attempt.hostRenderCountBefore << ",\"hostRenderCountAfter\":" << attempt.hostRenderCountAfter
                << ",\"primary\":{\"before\":";
            WriteEditorPreparationState(out, attempt.primaryBefore);
            out << ",\"after\":";
            WriteEditorPreparationState(out, attempt.primaryAfter);
            out << "},\"mirror\":{\"before\":";
            WriteEditorPreparationState(out, attempt.mirrorBefore);
            out << ",\"after\":";
            WriteEditorPreparationState(out, attempt.mirrorAfter);
            out << "}}";
        }
        out << "]}";
    }
    out << "]}";
}
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626)
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
#pragma warning(pop)
struct Memory
{
    PROCESS_MEMORY_COUNTERS_EX process{};
    DWORD handles{}, gdi{}, user{}, threads{};
    std::uint64_t cpu100ns{}, collectionCpu100ns{};
    double collectionUs{};
    bool threadInventoryCollected{};
};
Memory Snapshot(bool collectThreads = false)
{
    Memory value;
    const auto started = Clock::now();
    const auto cpu = ProcessCpu100ns();
    value.process.cb = sizeof(value.process);
    Require(GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&value.process), sizeof(value.process)) != FALSE,
            "process memory unavailable");
    Require(GetProcessHandleCount(GetCurrentProcess(), &value.handles) != FALSE, "process handle count unavailable");
    value.gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    value.user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    if (collectThreads)
    {
        wil::unique_handle all(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0));
        Require(static_cast<bool>(all), "thread inventory unavailable");
        THREADENTRY32 entry{sizeof(entry)};
        if (Thread32First(all.get(), &entry))
            do
            {
                if (entry.th32OwnerProcessID == GetCurrentProcessId())
                    ++value.threads;
            } while (Thread32Next(all.get(), &entry));
        value.threadInventoryCollected = true;
    }
    value.cpu100ns = ProcessCpu100ns();
    value.collectionCpu100ns = value.cpu100ns - cpu;
    value.collectionUs = Micros(started);
    return value;
}
void MemoryJson(std::ostream &out, const Memory &value)
{
    out << "\"privateBytes\":" << value.process.PrivateUsage << ",\"workingSetBytes\":" << value.process.WorkingSetSize
        << ",\"handles\":" << value.handles << ",\"gdiObjects\":" << value.gdi << ",\"userObjects\":" << value.user << ",\"threads\":";
    if (value.threadInventoryCollected)
        out << value.threads;
    else
        out << "null";
    out << ",\"threadInventoryCollected\":" << (value.threadInventoryCollected ? "true" : "false") << ",\"processCpu100ns\":" << value.cpu100ns
        << ",\"snapshotCollectionCpu100ns\":" << value.collectionCpu100ns << ",\"snapshotCollectionUs\":" << value.collectionUs;
}
} // namespace
// Existing identity/RAII/reference traits intentionally delete these implicit operations.
// Preserve aggregate and overload behavior. Owner: repository maintainers; review: 2027-01-31.
#pragma warning(push)
#pragma warning(disable : 4625 4626 5027)
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
            if (!app.Host().DispatchKeyboardMessage(message))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
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
    std::size_t Ready(std::wstring_view expected)
    {
        return ReadyImpl<false>(expected, nullptr, nullptr);
    }
    std::size_t Ready(std::wstring_view expected, EditorPreparationDiagnostics &preparationDiagnostics, FrameMetricBatch &frameMetrics)
    {
        return ReadyImpl<true>(expected, &preparationDiagnostics, &frameMetrics);
    }
    template <bool CaptureEditorPreparation>
    std::size_t ReadyImpl(std::wstring_view expected, EditorPreparationDiagnostics *preparationDiagnostics, FrameMetricBatch *frameMetrics)
    {
        std::size_t attempts{};
        const auto deadline = GetTickCount64() + 5000;
        do
        {
            ++attempts;
            bool frameSubmitted{};
            if constexpr (CaptureEditorPreparation)
            {
                EditorPreparationAttempt preparationAttempt{};
                preparationAttempt.frameMetricAttemptIndex = frameMetrics->frameCount;
                preparationAttempt.hostRenderCountBefore = app.host.DebugGetRenderCount();
                preparationAttempt.primaryBefore = app.editor->DebugGetState();
                preparationAttempt.mirrorBefore = app.mirror->DebugGetState();
                frameSubmitted = app.host.RenderInitialFrameForShow();
                preparationAttempt.hostRenderCountAfter = app.host.DebugGetRenderCount();
                preparationAttempt.primaryAfter = app.editor->DebugGetState();
                preparationAttempt.mirrorAfter = app.mirror->DebugGetState();
                preparationDiagnostics->Record(preparationAttempt);
            }
            else
                frameSubmitted = app.host.RenderInitialFrameForShow();
            Require(frameSubmitted, "native frame submission failed");
            const auto processed = Pump();
            const auto semantic = app.editor->GetAcknowledgedSemanticDocument(app.host);
            const auto mirror = app.mirror->DebugGetState();
            if (!app.host.HasPendingContentPreparation() && semantic && semantic->Syntax()->Source() == expected &&
                app.editor->GetImageResourceStatistics().pendingSources == 0 &&
                (app.settings.view != EditorView::Split || (!mirror.preparationPending && mirror.layoutReady)) && !app.files.Busy() &&
                app.recovery.Ready())
            {
                Require(app.host.DebugGetPresentFailureCount() == 0, "native presentation reported a failed frame");
                return attempts;
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
    void SettleRecovery()
    {
        const auto deadline = GetTickCount64() + 5000;
        while (!app.recovery.Settled())
        {
            Pump();
            Require(GetTickCount64() < deadline, "memory observation did not reach settled recovery");
            (void)MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
        Pump();
        Require(SUCCEEDED(app.RecoveryStatus()), "recovery failed before memory observation");
    }
    struct OperationCost
    {
        double wallUs{};
        std::uint64_t processCpu100ns{}, ownerCpuCycles{};
        std::uint64_t rootChildFlushes{};
    };
    OperationCost MeasureIdle(DWORD milliseconds)
    {
        const auto cpu = ProcessCpu100ns(), cycles = OwnerCycles();
        const auto started = Clock::now();
        Idle(milliseconds);
        OperationCost result{Micros(started), ProcessCpu100ns() - cpu, OwnerCycles() - cycles};
        Require(result.wallUs >= milliseconds * 900.0, "idle interval ended before its observation window");
        return result;
    }
    // Resource diagnostics time the complete native preparation and GPU fence;
    // they are separate from the unchanged paired edit/frame metric family.
    template <typename Action> OperationCost MeasureCompleted(Action &&action)
    {
        const auto cpu = ProcessCpu100ns(), cycles = OwnerCycles();
        const auto flushes = app.host.DebugGetInitialRootChildBatchFlushCount();
        const auto started = Clock::now();
        action();
        gpu->Wait();
        OperationCost result{Micros(started), ProcessCpu100ns() - cpu, OwnerCycles() - cycles,
                             app.host.DebugGetInitialRootChildBatchFlushCount() - flushes};
        Require(result.wallUs > 0, "completed native resource preparation must have a positive observation interval");
        return result;
    }
    static void CompletedCostJson(std::ostream &out, const OperationCost &cost)
    {
        out << "\"completedWallUs\":" << cost.wallUs << ",\"processCpu100ns\":" << cost.processCpu100ns
            << ",\"ownerCpuCycles\":" << cost.ownerCpuCycles << ",\"rootChildFlushes\":" << cost.rootChildFlushes;
    }
    void Resource(std::ostream &out, std::string_view name, const OperationCost *idle = nullptr, const OperationCost *completed = nullptr)
    {
        out << "{\"phase\":\"" << name << "\",";
        MemoryJson(out, Snapshot(true));
        if (idle)
            out << ",\"idleWallUs\":" << idle->wallUs << ",\"idleProcessCpu100ns\":" << idle->processCpu100ns
                << ",\"idleOwnerCpuCycles\":" << idle->ownerCpuCycles;
        if (completed)
        {
            out << ",\"preparation\":{";
            CompletedCostJson(out, *completed);
            out << '}';
        }
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

    template <typename App> static void WriteAppBuffers(std::ostream &out, const App &application)
    {
        bool first = true;
        const auto prefix = [&](const char *name, bool available)
        {
            if (!first)
                out << ',';
            first = false;
            out << '"' << name << "Available\":" << (available ? "true" : "false") << ",\"" << name << "\":";
        };
        const auto number = [&](const char *name, auto value)
        {
            prefix(name, true);
            out << value;
        };
        const auto boolean = [&](const char *name, bool value)
        {
            prefix(name, true);
            out << (value ? "true" : "false");
        };
        const auto unavailable = [&](const char *name)
        {
            prefix(name, false);
            out << "null";
        };
        out << '{';
        if constexpr (requires { application.documentInfo.characters; })
            number("sourceUnits", application.documentInfo.characters);
        else
            unavailable("sourceUnits");
        if constexpr (requires {
                          application.savedText.size();
                          application.savedText.capacity();
                      })
        {
            number("savedTextUnits", application.savedText.size());
            number("savedTextCapacityUnits", application.savedText.capacity());
        }
        else
        {
            unavailable("savedTextUnits");
            unavailable("savedTextCapacityUnits");
        }
        if constexpr (requires {
                          application.cachedText.size();
                          application.cachedText.capacity();
                      })
        {
            number("cachedTextUnits", application.cachedText.size());
            number("cachedTextCapacityUnits", application.cachedText.capacity());
        }
        else
        {
            unavailable("cachedTextUnits");
            unavailable("cachedTextCapacityUnits");
        }
        if constexpr (requires {
                          application.documentInfo.lineIndex.size();
                          application.documentInfo.lineIndex.capacity();
                      })
        {
            number("lineIndexItems", application.documentInfo.lineIndex.size());
            number("lineIndexCapacityItems", application.documentInfo.lineIndex.capacity());
        }
        else
        {
            unavailable("lineIndexItems");
            unavailable("lineIndexCapacityItems");
        }
        if constexpr (requires {
                          application.documentInfo.headings.size();
                          application.documentInfo.headings.capacity();
                          application.documentInfo.headings[std::size_t{}].text.size();
                          application.documentInfo.headings[std::size_t{}].text.capacity();
                      })
        {
            std::size_t textUnits{}, textCapacityUnits{};
            for (const auto &heading : application.documentInfo.headings)
            {
                textUnits += heading.text.size();
                textCapacityUnits += heading.text.capacity();
            }
            number("headingItems", application.documentInfo.headings.size());
            number("headingCapacityItems", application.documentInfo.headings.capacity());
            number("headingTextUnits", textUnits);
            number("headingTextCapacityUnits", textCapacityUnits);
        }
        else
        {
            unavailable("headingItems");
            unavailable("headingCapacityItems");
            unavailable("headingTextUnits");
            unavailable("headingTextCapacityUnits");
        }
        if constexpr (requires {
                          application.visibleFilePaths.size();
                          application.visibleFilePaths.capacity();
                      })
        {
            number("visibleFilePathItems", application.visibleFilePaths.size());
            number("visibleFilePathCapacityItems", application.visibleFilePaths.capacity());
        }
        else
        {
            unavailable("visibleFilePathItems");
            unavailable("visibleFilePathCapacityItems");
        }
        if constexpr (requires {
                          application.imageRequests.size();
                          application.imageRequests.capacity();
                      })
        {
            number("imageRequestItems", application.imageRequests.size());
            number("imageRequestCapacityItems", application.imageRequests.capacity());
        }
        else
        {
            unavailable("imageRequestItems");
            unavailable("imageRequestCapacityItems");
        }
        if constexpr (requires {
                          application.cachedSource.has_value();
                          application.cachedSource->Length();
                      })
        {
            boolean("cachedSourcePresent", application.cachedSource.has_value());
            number("cachedSourceUnits", application.cachedSource ? application.cachedSource->Length() : 0);
        }
        else
        {
            unavailable("cachedSourcePresent");
            unavailable("cachedSourceUnits");
        }
        if constexpr (requires {
                          application.mirroredSource.has_value();
                          application.mirroredSource->Length();
                      })
        {
            boolean("mirroredSourcePresent", application.mirroredSource.has_value());
            number("mirroredSourceUnits", application.mirroredSource ? application.mirroredSource->Length() : 0);
        }
        else
        {
            unavailable("mirroredSourcePresent");
            unavailable("mirroredSourceUnits");
        }
        if constexpr (requires {
                          application.selectionFactsSource.has_value();
                          application.selectionFactsSource->Length();
                      })
        {
            boolean("selectionFactsSourcePresent", application.selectionFactsSource.has_value());
            number("selectionFactsSourceUnits", application.selectionFactsSource ? application.selectionFactsSource->Length() : 0);
        }
        else
        {
            unavailable("selectionFactsSourcePresent");
            unavailable("selectionFactsSourceUnits");
        }
        if constexpr (requires {
                          application.documentAnalysisSource.has_value();
                          application.documentAnalysisSource->Length();
                      })
        {
            boolean("documentAnalysisSourcePresent", application.documentAnalysisSource.has_value());
            number("documentAnalysisSourceUnits", application.documentAnalysisSource ? application.documentAnalysisSource->Length() : 0);
        }
        else
        {
            unavailable("documentAnalysisSourcePresent");
            unavailable("documentAnalysisSourceUnits");
        }
        if constexpr (requires {
                          application.committedAnalysisSource.has_value();
                          application.committedAnalysisSource->Length();
                      })
        {
            boolean("committedAnalysisSourcePresent", application.committedAnalysisSource.has_value());
            number("committedAnalysisSourceUnits", application.committedAnalysisSource ? application.committedAnalysisSource->Length() : 0);
        }
        else
        {
            unavailable("committedAnalysisSourcePresent");
            unavailable("committedAnalysisSourceUnits");
        }
        if constexpr (requires {
                          application.selectionFactsSyntax->Nodes().size();
                          application.selectionFactsSyntax->Inlines().size();
                      })
        {
            number("selectionSyntaxNodes", application.selectionFactsSyntax ? application.selectionFactsSyntax->Nodes().size() : 0);
            number("selectionSyntaxInlines", application.selectionFactsSyntax ? application.selectionFactsSyntax->Inlines().size() : 0);
        }
        else
        {
            unavailable("selectionSyntaxNodes");
            unavailable("selectionSyntaxInlines");
        }
        out << '}';
    }

  public:
    PerformanceProbe(Application &value, const LaunchOptions &launch) : app(value), options(launch) {}
    int Run()
    {
        // Freeze client pixels before warmup. Native and merged non-client
        // chrome reserve different frame extents; measuring outer-window sizes
        // would silently compare different document viewports.
        const auto dpi = GetDpiForWindow(app.Window());
        const int targetWidth = MulDiv(1440, static_cast<int>(dpi), 96), targetHeight = MulDiv(900, static_cast<int>(dpi), 96);
        bool sized{};
        for (unsigned attempt = 0; attempt < 3; ++attempt)
        {
            RECT client{}, frame{};
            Require(GetClientRect(app.Window(), &client) && GetWindowRect(app.Window(), &frame), "read owned performance viewport");
            if (client.right == targetWidth && client.bottom == targetHeight)
            {
                sized = true;
                break;
            }
            Require(SetWindowPos(app.Window(), nullptr, 0, 0, frame.right - frame.left + targetWidth - client.right,
                                 frame.bottom - frame.top + targetHeight - client.bottom,
                                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING) != FALSE,
                    "normalize the owned hidden performance viewport");
            Pump();
        }
        RECT actualViewport{};
        Require(GetClientRect(app.Window(), &actualViewport), "read final performance viewport");
        Require(sized && !IsWindowVisible(app.Window()),
                std::format("performance viewport must be hidden and exactly {}x{} pixels (1440x900 DIP at {} DPI); actual {}x{}, visible={}",
                            targetWidth, targetHeight, dpi, actualViewport.right, actualViewport.bottom, IsWindowVisible(app.Window()))
                    .c_str());
        app.settings.appearance = Appearance::Light;
        app.ApplyTheme();
        app.SetView(options.scenario == L"split" ? EditorView::Split : EditorView::Compose);
        const auto source = app.editor->GetText();
        const auto at = source.find(L"S5_EDIT_HERE");
        Require(at != source.npos, "paired fixture is missing its fixed prose edit site");
        auto edited = source;
        edited.insert(at, 1, L'x');
        const auto prepareInitialFrame = [&]
        {
            Ready(source);
            gpu = std::make_unique<GpuFence>(app.host);
            Require(!options.forceWarp || (gpu->adapter.Flags & DXGI_ADAPTER_FLAG_SOFTWARE), "explicit WARP did not create a software adapter");
        };
        OperationCost firstCompletedPreparation{};
        if (options.resourceProbe)
            firstCompletedPreparation = MeasureCompleted(prepareInitialFrame);
        else
        {
            prepareInitialFrame();
            gpu->Wait();
        }
        std::vector<double> command, preparation, completion, total, clean, cpu, commandCpu, processorStart, processorEnd;
        std::array<std::vector<double>, EditDiagnostics::Count> phaseWall, phaseCpu;
        std::vector<EditDiagnostics::SelectionRecord> selectionComponents;
        std::vector<double> preparationCpu, completionCpu, renderCounts, preparationCounts;
        std::vector<FrameMetricBatch> editFrameMetrics, cleanFrameMetrics;
        EditorPreparationDiagnostics editorPreparationDiagnostics;
        for (auto *values : {&command, &preparation, &completion, &total, &clean, &cpu, &commandCpu, &processorStart, &processorEnd})
            values->reserve(options.performanceCycles * 2);
        if (options.controlDiagnostics)
        {
            for (auto *phases : {&phaseWall, &phaseCpu})
                for (auto &values : *phases)
                    values.reserve(options.performanceCycles * 2);
            for (auto *values : {&preparationCpu, &completionCpu, &renderCounts, &preparationCounts})
                values->reserve(options.performanceCycles * 2);
            selectionComponents.reserve(options.performanceCycles * 2);
            editFrameMetrics.reserve(options.performanceCycles * 2);
            cleanFrameMetrics.reserve(options.performanceCycles);
            editorPreparationDiagnostics.Reserve(options.performanceCycles * 2);
            Require(editFrameMetrics.capacity() == options.performanceCycles * 2 && cleanFrameMetrics.capacity() == options.performanceCycles,
                    "frame diagnostic vector capacities exceed the declared retention bound");
            Require(FrameMetricCollectorOracle(), "frame metric collector oracle failed");
            Require(EditorPreparationDiagnosticsOracle(), "editor preparation diagnostic oracle failed");
        }
        const auto activity = app.activity;
        SettleRecovery();
        const auto initial = Snapshot();
        for (unsigned cycle = 0; cycle < options.performanceCycles + 2; ++cycle)
        {
            app.editor->SetSelection({at, at});
            for (unsigned undo = 0; undo < 2; ++undo)
            {
                EditDiagnostics diagnostic;
                app.editDiagnostics = options.controlDiagnostics ? &diagnostic : nullptr;
                const auto restoreDiagnostics = wil::scope_exit([&] { app.editDiagnostics = nullptr; });
                const auto renders = app.host.DebugGetRenderCount();
                const auto advances = app.editor->DebugGetState().preparationAdvances;
                const auto startingProcessor = OwnerProcessor();
                std::unique_ptr<FrameMetricBatch> frameMetrics;
                std::optional<FrameMetricSinkScope> frameSink;
                if (options.controlDiagnostics)
                {
                    frameMetrics = std::make_unique<FrameMetricBatch>();
                    frameSink.emplace(frameMetrics.get());
                }
                const auto start = Clock::now();
                const auto cpuStart = OwnerCycles();
                if (options.controlDiagnostics)
                    diagnostic.Begin();
                Require(undo ? app.editor->Undo() : app.editor->ReplaceSelection(L"x"), "paired edit/undo was not admitted");
                if (options.controlDiagnostics)
                    diagnostic.Mark(EditPhase::Exit);
                app.editDiagnostics = nullptr;
                const double commandUs = Micros(start);
                const auto commandCycles = OwnerCycles() - cpuStart;
                const auto prepare = Clock::now();
                const bool collectEditorPreparation = options.controlDiagnostics && cycle >= 2;
                const std::wstring_view expected = undo ? std::wstring_view(source) : std::wstring_view(edited);
                std::size_t requestedAttempts{};
                if (collectEditorPreparation)
                {
                    editorPreparationDiagnostics.BeginTransaction();
                    requestedAttempts = Ready(expected, editorPreparationDiagnostics, *frameMetrics);
                }
                else
                    requestedAttempts = Ready(expected);
                if (collectEditorPreparation)
                    editorPreparationDiagnostics.EndTransaction(requestedAttempts);
                const double prepareUs = Micros(prepare);
                const auto prepareCycles = options.controlDiagnostics ? OwnerCycles() - cpuStart - commandCycles : 0;
                const auto finish = Clock::now();
                gpu->Wait();
                const double gpuUs = Micros(finish), totalUs = Micros(start);
                const auto cpuCycles = OwnerCycles() - cpuStart;
                const auto endingProcessor = OwnerProcessor();
                const auto frameRenderCount = options.controlDiagnostics ? app.host.DebugGetRenderCount() - renders : 0;
                if (frameMetrics)
                {
                    frameMetrics->hostRenderCount = frameRenderCount;
                    frameMetrics->requestedAttempts = requestedAttempts;
                }
                if (frameSink)
                    frameSink->Reset();
                if (options.controlDiagnostics && !frameMetrics->Complete())
                {
                    std::ostringstream reason;
                    reason << "edit DxUi frame metrics: frames=" << frameMetrics->frameCount << " host=" << frameRenderCount
                           << " requested=" << requestedAttempts << " next=" << frameMetrics->nextMetric << " valid=" << frameMetrics->valid;
                    for (std::size_t index = 0; index < frameMetrics->frameCount; ++index)
                    {
                        const auto &frame = frameMetrics->frames[index];
                        reason << " [" << frame.totalUs << ',' << frame.updateUs << ',' << frame.renderUs << ',' << frame.presentUs << ']';
                    }
                    throw std::runtime_error(reason.str());
                }
                Require(!options.controlDiagnostics || diagnostic.Complete(), "command diagnostic phases did not close in order");
                if (cycle >= 2)
                {
                    command.push_back(commandUs);
                    preparation.push_back(prepareUs);
                    completion.push_back(gpuUs);
                    total.push_back(totalUs);
                    cpu.push_back(static_cast<double>(cpuCycles));
                    commandCpu.push_back(static_cast<double>(commandCycles));
                    processorStart.push_back(startingProcessor);
                    processorEnd.push_back(endingProcessor);
                    if (options.controlDiagnostics)
                    {
                        for (unsigned index = 0; index < EditDiagnostics::Count; ++index)
                        {
                            phaseWall[index].push_back(diagnostic.wallUs[index]);
                            phaseCpu[index].push_back(static_cast<double>(diagnostic.cpuCycles[index]));
                        }
                        preparationCpu.push_back(static_cast<double>(prepareCycles));
                        completionCpu.push_back(static_cast<double>(cpuCycles - commandCycles - prepareCycles));
                        renderCounts.push_back(static_cast<double>(app.host.DebugGetRenderCount() - renders));
                        preparationCounts.push_back(static_cast<double>(app.editor->DebugGetState().preparationAdvances - advances));
                        selectionComponents.push_back(diagnostic.selection);
                        editFrameMetrics.push_back(*frameMetrics);
                    }
                }
            }
            Require(app.editor->GetText() == source && app.editor->GetSelection() == DxUi::RichText::Selection{at, at},
                    "paired undo changed source or selection");
            if (!options.controlDiagnostics)
            {
                const auto start = Clock::now();
                Require(app.host.RenderInitialFrameForShow(), "clean native frame failed");
                gpu->Wait();
                if (cycle >= 2)
                    clean.push_back(Micros(start));
            }
            else
            {
                const auto renders = app.host.DebugGetRenderCount();
                auto frameMetrics = std::make_unique<FrameMetricBatch>();
                FrameMetricSinkScope frameSink(frameMetrics.get());
                const auto start = Clock::now();
                Require(app.host.RenderInitialFrameForShow(), "clean native frame failed");
                gpu->Wait();
                const auto cleanUs = Micros(start);
                const auto frameRenderCount = app.host.DebugGetRenderCount() - renders;
                frameMetrics->hostRenderCount = frameRenderCount;
                frameMetrics->requestedAttempts = 1;
                frameSink.Reset();
                Require(frameMetrics->Complete(), "clean DxUi frame metrics did not match the host render count or had invalid timing");
                if (cycle >= 2)
                {
                    clean.push_back(cleanUs);
                    cleanFrameMetrics.push_back(*frameMetrics);
                }
            }
        }
        if (options.controlDiagnostics)
            Require(editFrameMetrics.size() == options.performanceCycles * 2 && cleanFrameMetrics.size() == options.performanceCycles &&
                        editorPreparationDiagnostics.Complete(std::span<const FrameMetricBatch>(editFrameMetrics), options.performanceCycles * 2),
                    "frame diagnostics must retain every measured edit and clean transaction");
        SettleRecovery();
        Require(app.editor->GetText() == source && app.editor->GetSelection() == DxUi::RichText::Selection{at, at},
                "settled memory observation changed source or selection");
        const auto final = Snapshot();
        std::array<Memory, 3> postMemory;
        if (options.controlDiagnostics)
        {
            const auto renders = app.host.DebugGetRenderCount(), advances = app.editor->DebugGetState().preparationAdvances;
            postMemory[0] = Snapshot();
            Idle(100);
            postMemory[1] = Snapshot();
            Idle(400);
            postMemory[2] = Snapshot(true); // Global inventory is diagnostic-only, after all primary observations.
            Require(app.host.DebugGetRenderCount() == renders && app.editor->DebugGetState().preparationAdvances == advances,
                    "post-observation hidden idle requested extra render/preparation");
        }
        std::ofstream out(options.performance, std::ios::binary);
        out << std::setprecision(12) << "{\"schemaVersion\":2,\"fixture\":\"redtilde-completed-native-v2\",\"view\":\""
            << (app.settings.view == EditorView::Split ? "Split" : "Compose") << "\",\"renderer\":\""
            << ((gpu->adapter.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ? "WARP" : "Hardware") << "\",\"adapter\":";
        Quote(out, gpu->adapter.Description);
        out << ",\"vendorId\":" << gpu->adapter.VendorId << ",\"deviceId\":" << gpu->adapter.DeviceId << ",\"driverVersion\":" << gpu->driver.QuadPart
            << ",\"dpi\":" << app.host.GetDpi() << ",\"widthPx\":" << app.host.DebugGetWidthPx() << ",\"heightPx\":" << app.host.DebugGetHeightPx()
            << ",\"sourceUnits\":" << source.size() << ",\"cycles\":" << options.performanceCycles
            << ",\"warmupCycles\":2,\"recoverySettledAtMemory\":true";
        if (options.resourceProbe)
        {
            out << ",\"firstCompletedPreparation\":{";
            CompletedCostJson(out, firstCompletedPreparation);
            out << '}';
        }
        out << ",\"samples\": {";
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
        out << ',';
        Samples(out, "commandOwnerCpuCycles", commandCpu);
        out << ',';
        Samples(out, "ownerProcessorStart", processorStart);
        out << ',';
        Samples(out, "ownerProcessorEnd", processorEnd);
        out << "},\"initial\":{";
        MemoryJson(out, initial);
        out << "},\"final\":{";
        MemoryJson(out, final);
        out << "},";
        if (options.controlDiagnostics)
        {
            out << "\"appBuffersAfterPrimary\":";
            WriteAppBuffers(out, app);
            out << ',';
        }
        out << "\"controlDiagnostics\":";
        if (options.controlDiagnostics)
        {
            constexpr std::array names{"enter", "analysis", "mirror", "identity", "caption", "chrome", "selection", "recovery", "exit"};
            out << "{\"protocol\":\"app-edit-phases-v2\",\"primaryBeforeInventory\":true,\"phases\":[";
            for (unsigned index = 0; index < EditDiagnostics::Count; ++index)
            {
                if (index)
                    out << ',';
                out << "{\"name\":\"" << names[index] << "\",";
                Samples(out, "wallUs", phaseWall[index]);
                out << ',';
                Samples(out, "ownerCpuCycles", phaseCpu[index]);
                out << '}';
            }
            out << "],";
            Samples(out, "preparationOwnerCpuCycles", preparationCpu);
            out << ',';
            Samples(out, "gpuWaitOwnerCpuCycles", completionCpu);
            out << ',';
            Samples(out, "renderCounts", renderCounts);
            out << ',';
            Samples(out, "preparationAdvances", preparationCounts);
            constexpr std::array selectionNames{"selection-snapshot",
                                                "document-analysis-refresh",
                                                "selection-facts-cache",
                                                "mirror",
                                                "inline-semantics",
                                                "tool-caption-publication",
                                                "outline",
                                                "block-lookup",
                                                "block-labels",
                                                "position-status",
                                                "find",
                                                "routing"};
            out << ",\"selectionComponents\":{\"protocol\":\"app-selection-components-v1\",\"transactions\":[";
            for (unsigned transaction = 0; transaction < selectionComponents.size(); ++transaction)
            {
                if (transaction)
                    out << ',';
                const auto &record = selectionComponents[transaction];
                double measuredWall{}, measuredCycles{};
                for (unsigned component = 0; component < EditDiagnostics::SelectionCount; ++component)
                {
                    measuredWall += record.wallUs[component];
                    measuredCycles += static_cast<double>(record.cpuCycles[component]);
                }
                out << "{\"complete\":" << (record.complete ? "true" : "false") << ",\"invocations\":" << record.invocations
                    << ",\"wallTotalUs\":" << record.wallTotalUs << ",\"ownerCpuCyclesTotal\":" << record.cpuTotalCycles << ",\"components\":[";
                for (unsigned component = 0; component < EditDiagnostics::SelectionCount; ++component)
                {
                    if (component)
                        out << ',';
                    out << "{\"name\":\"" << selectionNames[component] << "\",\"calls\":" << record.calls[component] << ",\"wallUs\":";
                    if (record.calls[component])
                        out << record.wallUs[component];
                    else
                        out << "null";
                    out << ",\"ownerCpuCycles\":";
                    if (record.calls[component])
                        out << record.cpuCycles[component];
                    else
                        out << "null";
                    out << '}';
                }
                const auto parentWall = phaseWall[static_cast<unsigned>(EditPhase::Selection)][transaction];
                const auto parentCycles = phaseCpu[static_cast<unsigned>(EditPhase::Selection)][transaction];
                out << "],\"parentReconciliation\":{\"wallUs\":" << parentWall - measuredWall
                    << ",\"ownerCpuCycles\":" << parentCycles - measuredCycles << "}}";
            }
            out << "]},";
            out << "\"postMemory\":[";
            for (unsigned index = 0; index < postMemory.size(); ++index)
            {
                if (index)
                    out << ',';
                out << '{';
                MemoryJson(out, postMemory[index]);
                out << '}';
            }
            out << "]}";
        }
        else
            out << "null";
        out << ",\"frameDiagnostics\":";
        if (options.controlDiagnostics)
        {
            const auto reservedBytes = (editFrameMetrics.capacity() + cleanFrameMetrics.capacity()) * sizeof(FrameMetricBatch);
            const auto capacityBoundBytes = (static_cast<std::size_t>(options.performanceCycles) * 3) * sizeof(FrameMetricBatch);
            Require(reservedBytes == capacityBoundBytes, "frame diagnostic storage exceeds the declared capacity bound");
            out << "{\"protocol\":\"dxui-frame-metrics-v1\",\"source\":\"DxUi.FrameRuntime.FrameMetricSink\",\"units\":\"microseconds\","
                   "\"wallDurationOnly\":true,\"storage\":{\"reservedBytes\":"
                << reservedBytes << ",\"capacityBoundBytes\":" << capacityBoundBytes << ",\"editTransactionCapacity\":" << editFrameMetrics.capacity()
                << ",\"cleanTransactionCapacity\":" << cleanFrameMetrics.capacity()
                << ",\"maxTransactions\":" << static_cast<std::size_t>(options.performanceCycles) * 3
                << ",\"maxAttemptsPerTransaction\":" << FrameMetricCapacity << ",\"activeBatchBytes\":" << sizeof(FrameMetricBatch)
                << ",\"postMemoryIncludesReservedBytes\":true,\"allocatorMetadataIncluded\":false},";
            WriteFrameMetricTransactions(out, "editTransactions", editFrameMetrics);
            out << ",";
            WriteFrameMetricTransactions(out, "cleanTransactions", cleanFrameMetrics);
            out << "}";
        }
        else
            out << "null";
        out << ",\"editorPreparationDiagnostics\":";
        if (options.controlDiagnostics)
            WriteEditorPreparationDiagnostics(out, editorPreparationDiagnostics);
        else
            out << "null";
        out << ",\"activity\":{\"changes\":" << app.activity.changes - activity.changes << ",\"layouts\":" << app.activity.layouts - activity.layouts
            << ",\"mirrorResets\":" << app.activity.mirrorResets - activity.mirrorResets
            << ",\"recoverySubmissions\":" << app.activity.recoverySubmissions - activity.recoverySubmissions
            << ",\"captionRequests\":" << app.activity.captionRequests - activity.captionRequests
            << ",\"captionCalls\":" << app.activity.captionCalls - activity.captionCalls << "},\"resources\":[";
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
            const auto cleanIdle = MeasureIdle(500);
            Require(app.host.DebugGetRenderCount() == renders && app.editor->DebugGetState().preparationAdvances == advances,
                    "clean hidden idle performed unrequested rendering or preparation");
            out << ',';
            Resource(out, "clean-hidden-idle", &cleanIdle);
            (void)SendMessageW(app.window.get(), WM_SHOWWINDOW, FALSE, 0);
            Pump();
            const auto hidden = app.host.DebugGetRenderCount();
            const auto hiddenIdle = MeasureIdle(500);
            Require(!IsWindowVisible(app.window.get()) && app.host.DebugGetRenderCount() == hidden, "hidden notification did not quiesce rendering");
            out << ',';
            Resource(out, "hidden-notification-idle", &hiddenIdle);
            (void)SendMessageW(app.window.get(), WM_SHOWWINDOW, TRUE, 0);
            Ready(resourceSource);
            gpu->Wait();
            out << ',';
            Resource(out, "resumed-forced-frame");
            RECT outer{};
            Require(GetWindowRect(app.window.get(), &outer) != FALSE, "native resource window bounds unavailable");
            const auto resizeCost = MeasureCompleted(
                [&]
                {
                    Require(SetWindowPos(app.window.get(), nullptr, 0, 0, outer.right - outer.left - 120, outer.bottom - outer.top - 80,
                                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSENDCHANGING) != FALSE,
                            "native resource resize failed");
                    Ready(resourceSource);
                });
            out << ',';
            Resource(out, "resized-forced-frame", nullptr, &resizeCost);
            Require(SetWindowPos(app.window.get(), nullptr, 0, 0, outer.right - outer.left, outer.bottom - outer.top,
                                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSENDCHANGING) != FALSE,
                    "native resource size restore failed");
            Ready(resourceSource);
            gpu->Wait();
            RECT restoredViewport{};
            Require(GetClientRect(app.Window(), &restoredViewport) && restoredViewport.right == targetWidth &&
                        restoredViewport.bottom == targetHeight,
                    "native resource restore must preserve the exact normalized client viewport");
            const auto before = options.dataDirectory / L"resource-before.png";
            Require(app.Capture(before), "pre-replacement native capture failed");
            std::string pixels;
            Check(ReadBytes(before, pixels), "pre-replacement capture unavailable");
            for (unsigned cycle = 0; cycle < 3; ++cycle)
            {
                const auto generation = app.host.GetResourceGeneration();
                const auto recreateCost = MeasureCompleted(
                    [&]
                    {
                        gpu.reset();
                        app.host.DebugSimulateDeviceLoss();
                        Ready(resourceSource);
                        gpu = std::make_unique<GpuFence>(app.host);
                    });
                Require(app.host.GetResourceGeneration() != generation, "device replacement did not advance generation");
                const auto after = options.dataDirectory / (L"resource-after-" + std::to_wstring(cycle) + L".png");
                Require(app.Capture(after), "post-replacement native capture failed");
                std::string actual;
                Check(ReadBytes(after, actual), "post-replacement capture unavailable");
                Require(actual == pixels, "device replacement changed exact native pixels");
                out << ',';
                Resource(out, "recreated-forced-frame", nullptr, &recreateCost);
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
            MemoryJson(out, Snapshot(true));
            out << ",\"nativeHosts\":" << DxUi::DebugGetAttachedWindowHostCount() << '}';
        }
        out << ']';
        if (options.controlDiagnostics)
        {
            std::array<HANDLE, 2> ownedHeaps{};
            std::size_t ownedHeapCount{};
            const auto defaultHeap = GetProcessHeap();
            if (defaultHeap != nullptr)
                ownedHeaps[ownedHeapCount++] = defaultHeap;
            const auto crtHeapValue = _get_heap_handle();
            if (crtHeapValue != 0 && crtHeapValue != static_cast<decltype(crtHeapValue)>(-1))
            {
                const auto crtHeap = reinterpret_cast<HANDLE>(crtHeapValue);
                if (crtHeap != nullptr &&
                    std::find(ownedHeaps.data(), ownedHeaps.data() + ownedHeapCount, crtHeap) == ownedHeaps.data() + ownedHeapCount)
                    ownedHeaps[ownedHeapCount++] = crtHeap;
            }
            out << ",\"heapDiagnosticCoverage\":\"explicit-default-and-crt-only\",\"ownedHeapDiagnostics\":";
            (void)Test::WriteOwnedHeapDiagnostic(out, std::span<const HANDLE>(ownedHeaps.data(), ownedHeapCount));
            const auto *const root = app.host.GetRoot();
            out << ",\"hostCacheCounters\":{\"scope\":\"after primary and heap; object counts, not allocation ownership\",\"attached\":"
                << (app.host.GetHwnd() ? "true" : "false") << ",\"rootLogicalChildren\":" << (root ? root->GetLogicalChildCount() : 0u)
                << ",\"configuredTextFormats\":" << app.host.DebugGetConfiguredTextFormatCount()
                << ",\"cachedBrushes\":" << app.host.DebugGetBrushCacheSize() << '}';
        }
        out << ",\"completed\":true,\"qualification\":\"hidden native completed frames; physical presentation and input unverified\"}\n";
        Require(static_cast<bool>(out), "native probe report write failed");
        return 0;
    }
};
#pragma warning(pop)
int RunPerformanceProbe(Application &app, const LaunchOptions &options)
{
    return PerformanceProbe(app, options).Run();
}
} // namespace RedTilde
