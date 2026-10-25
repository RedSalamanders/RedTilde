#pragma once

#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <ostream>
#include <span>
#include <wil/resource.h>

namespace RedTilde::Test
{
// This app-layer probe accepts only caller-owned heaps so a test can isolate a
// deterministic heap without enumerating or attributing unrelated process heaps.
// DxUi's analogous helper lives under its private Tests/Support tree; the app
// cannot include an SDK-private test header as a product or test dependency.
// This diagnostic runs outside timed frames and never purges heaps or trims the
// working set. It records aggregate heap state, not allocation ownership.
inline constexpr std::size_t MaximumDiagnosticHeaps = 32;
inline constexpr std::uint64_t MaximumDiagnosticEntriesPerHeap = 1'000'000;
// Each HeapWalk byte field is a DWORD, so 32 heaps * 1,000,000 entries *
// UINT32_MAX is below 2^57; uint64_t totals cannot saturate under these bounds.
static_assert(MaximumDiagnosticHeaps <=
              std::numeric_limits<std::uint64_t>::max() / MaximumDiagnosticEntriesPerHeap / std::numeric_limits<std::uint32_t>::max());

struct HeapDiagnosticSample
{
    std::uint64_t busyPayloadBytes{}, freePayloadBytes{}, entryOverheadBytes{};
    std::uint64_t regionCommittedBytes{}, regionUncommittedBytes{};
    std::uint64_t walkedEntries{};
    DWORD error = ERROR_SUCCESS;
    bool lockAcquired{}, complete{}, budgetExceeded{};
};

struct HeapDiagnosticTotals
{
    std::uint64_t busyPayloadBytes{}, freePayloadBytes{}, entryOverheadBytes{};
    std::uint64_t regionCommittedBytes{}, regionUncommittedBytes{};
};

inline void AddSaturated(std::uint64_t &total, std::uint64_t value) noexcept
{
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    total = value > maximum - total ? maximum : total + value;
}

inline bool ProcessCpu100ns(std::uint64_t &value) noexcept
{
    FILETIME created{}, exited{}, kernel{}, user{};
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) == FALSE)
        return false;
    value = (static_cast<std::uint64_t>(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) +
            (static_cast<std::uint64_t>(user.dwHighDateTime) << 32 | user.dwLowDateTime);
    return true;
}

// Walks no more than MaximumDiagnosticHeaps and MaximumDiagnosticEntriesPerHeap
// entries per heap. Heap handles must remain valid and owned by the caller for
// this synchronous call. The supplied span must not contain duplicate handles;
// duplicates are reported as invalid coverage. All stream formatting occurs
// outside HeapLock; while locked, this uses only stack storage and Win32 heap
// queries. Reported timing covers heap walks and counter reads only; it excludes
// JSON serialization and stream writes.
inline bool WriteOwnedHeapDiagnostic(std::ostream &out, std::span<const HANDLE> heaps, std::size_t entryLimit = MaximumDiagnosticEntriesPerHeap)
{
    LARGE_INTEGER frequency{}, start{}, finish{};
    const bool frequencyReady = QueryPerformanceFrequency(&frequency) != FALSE && frequency.QuadPart > 0;
    const bool startReady = frequencyReady && QueryPerformanceCounter(&start) != FALSE;
    std::uint64_t cpuStart{};
    const bool cpuStartReady = ProcessCpu100ns(cpuStart);

    std::array<HeapDiagnosticSample, MaximumDiagnosticHeaps> samples{};
    HeapDiagnosticTotals totals{};
    const auto count = std::min(heaps.size(), samples.size());
    const bool validEntryLimit = entryLimit > 0 && entryLimit <= MaximumDiagnosticEntriesPerHeap;
    bool complete = heaps.size() <= samples.size() && validEntryLimit;
    std::size_t attemptedHeapCount{}, actualWalkedHeapCount{}, completeHeapCount{};

    for (std::size_t i = 0; i < count; ++i)
    {
        auto &sample = samples[i];
        if (!validEntryLimit)
        {
            sample.error = ERROR_INVALID_PARAMETER;
            continue;
        }
        bool duplicateHandle{};
        for (std::size_t prior = 0; prior < i; ++prior)
            duplicateHandle = duplicateHandle || heaps[prior] == heaps[i];
        if (heaps[i] == nullptr || duplicateHandle)
        {
            sample.error = heaps[i] == nullptr ? ERROR_INVALID_HANDLE : ERROR_INVALID_PARAMETER;
            complete = false;
            continue;
        }
        ++attemptedHeapCount;
        if (HeapLock(heaps[i]) == FALSE)
        {
            sample.error = GetLastError();
            complete = false;
            continue;
        }

        sample.lockAcquired = true;
        ++actualWalkedHeapCount;
        {
            const auto unlock = wil::scope_exit(
                [&]
                {
                    if (HeapUnlock(heaps[i]) == FALSE)
                    {
                        sample.error = GetLastError();
                        sample.complete = false;
                    }
                });
            PROCESS_HEAP_ENTRY entry{};
            while (sample.walkedEntries < entryLimit && HeapWalk(heaps[i], &entry) != FALSE)
            {
                ++sample.walkedEntries;
                if ((entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0)
                {
                    AddSaturated(sample.busyPayloadBytes, entry.cbData);
                    AddSaturated(sample.entryOverheadBytes, entry.cbOverhead);
                }
                else if ((entry.wFlags & PROCESS_HEAP_REGION) != 0)
                {
                    AddSaturated(sample.regionCommittedBytes, entry.Region.dwCommittedSize);
                    AddSaturated(sample.regionUncommittedBytes, entry.Region.dwUnCommittedSize);
                }
                else if ((entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) == 0)
                {
                    AddSaturated(sample.freePayloadBytes, entry.cbData);
                    AddSaturated(sample.entryOverheadBytes, entry.cbOverhead);
                }
            }
            sample.error = GetLastError();
            if (sample.walkedEntries == entryLimit)
            {
                sample.budgetExceeded = HeapWalk(heaps[i], &entry) != FALSE;
                sample.error = sample.budgetExceeded ? ERROR_MORE_DATA : GetLastError();
            }
            const bool walkComplete = !sample.budgetExceeded && sample.error == ERROR_NO_MORE_ITEMS;
            if (walkComplete)
                sample.error = ERROR_SUCCESS;
            sample.complete = walkComplete;
        }
        complete = complete && sample.complete;
        if (sample.complete)
            ++completeHeapCount;
        AddSaturated(totals.busyPayloadBytes, sample.busyPayloadBytes);
        AddSaturated(totals.freePayloadBytes, sample.freePayloadBytes);
        AddSaturated(totals.entryOverheadBytes, sample.entryOverheadBytes);
        AddSaturated(totals.regionCommittedBytes, sample.regionCommittedBytes);
        AddSaturated(totals.regionUncommittedBytes, sample.regionUncommittedBytes);
    }

    std::uint64_t cpuFinish{};
    const bool cpuFinishReady = ProcessCpu100ns(cpuFinish);
    double wallMicroseconds{};
    const bool finishReady = startReady && QueryPerformanceCounter(&finish) != FALSE;
    const bool clockReady = finishReady && finish.QuadPart >= start.QuadPart;
    if (clockReady)
        wallMicroseconds = static_cast<double>(finish.QuadPart - start.QuadPart) * 1'000'000.0 / static_cast<double>(frequency.QuadPart);
    const bool cpuReady = cpuStartReady && cpuFinishReady && cpuFinish >= cpuStart;
    const auto cpu100ns = cpuReady ? cpuFinish - cpuStart : 0;

    out << "{\"complete\":" << (complete ? "true" : "false")
        << ",\"heapCoverage\":\"explicit provided unique handles only\",\"requestedHeapCount\":" << heaps.size() << ",\"boundedHeapCount\":" << count
        << ",\"attemptedHeapCount\":" << attemptedHeapCount << ",\"actualWalkedHeapCount\":" << actualWalkedHeapCount
        << ",\"completeHeapCount\":" << completeHeapCount << ",\"entryLimit\":" << entryLimit
        << ",\"costScope\":\"heap walk and counter reads only; excludes JSON serialization and stream writes\""
        << ",\"clockAvailable\":" << (clockReady ? "true" : "false") << ",\"wallMicroseconds\":" << wallMicroseconds
        << ",\"processCpuAvailable\":" << (cpuReady ? "true" : "false") << ",\"processCpu100ns\":" << cpu100ns
        << ",\"totals\":{\"busyPayloadBytes\":" << totals.busyPayloadBytes << ",\"freePayloadBytes\":" << totals.freePayloadBytes
        << ",\"entryOverheadBytes\":" << totals.entryOverheadBytes << ",\"regionCommittedBytes\":" << totals.regionCommittedBytes
        << ",\"regionUncommittedBytes\":" << totals.regionUncommittedBytes << "},\"heaps\":[";
    for (std::size_t i = 0; i < count; ++i)
    {
        if (i != 0)
            out << ',';
        const auto &sample = samples[i];
        out << "{\"index\":" << i << ",\"busyPayloadBytes\":" << sample.busyPayloadBytes << ",\"freePayloadBytes\":" << sample.freePayloadBytes
            << ",\"entryOverheadBytes\":" << sample.entryOverheadBytes << ",\"regionCommittedBytes\":" << sample.regionCommittedBytes
            << ",\"regionUncommittedBytes\":" << sample.regionUncommittedBytes << ",\"walkedEntries\":" << sample.walkedEntries
            << ",\"budgetExceeded\":" << (sample.budgetExceeded ? "true" : "false")
            << ",\"lockAcquired\":" << (sample.lockAcquired ? "true" : "false") << ",\"complete\":" << (sample.complete ? "true" : "false")
            << ",\"error\":" << sample.error << '}';
    }
    out << "]}";
    return complete;
}
} // namespace RedTilde::Test
