#pragma once
#include "Common/Recovery.h"
#include "Common/Storage.h"
#include <cstring>
#include <format>
#include <functional>
#include <stdexcept>
#include <wil/resource.h>

namespace RedTilde::Testing
{
inline void RequireStorage(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
// The startup scanner deliberately omits leased snapshots. These storage tests
// inspect complete envelopes directly to prove durability before mutation.
inline std::vector<RecoverySnapshot> ProtectedSaveSnapshots(const std::filesystem::path &dataDirectory)
{
    std::vector<RecoverySnapshot> values;
    const auto directory = dataDirectory / L"Recovery";
    if (!std::filesystem::exists(directory))
        return values;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
        if (entry.path().extension() == L".rtd")
        {
            std::string bytes;
            RecoverySnapshot value;
            RequireStorage(SUCCEEDED(ReadBytes(entry.path(), bytes)) && SUCCEEDED(ParseRecovery(bytes, value)), "complete save protection envelope");
            values.push_back(std::move(value));
        }
    return values;
}
struct LinkedSaveCheckpoint
{
    std::filesystem::path dataDirectory, newAlias;
    std::string observedBytes;
    std::vector<RecoverySnapshot> snapshots;
    HRESULT result = E_UNEXPECTED;
    unsigned protectedCalls{}, stagedCalls{};
    static void Observe(AtomicWriteCheckpoint phase, const std::filesystem::path &path, void *context) noexcept
    {
        // Test-hook ABI: report inspection failure without escaping into storage.
        try
        {
            auto &self = *static_cast<LinkedSaveCheckpoint *>(context);
            if (phase == AtomicWriteCheckpoint::Staged && !self.newAlias.empty())
            {
                ++self.stagedCalls;
                self.result = CreateHardLinkW(self.newAlias.c_str(), path.c_str(), nullptr) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
                self.newAlias.clear();
            }
            if (phase == AtomicWriteCheckpoint::InPlaceProtected)
            {
                ++self.protectedCalls;
                self.result = ReadBytes(path, self.observedBytes);
                self.snapshots = ProtectedSaveSnapshots(self.dataDirectory);
            }
        }
        catch (const std::bad_alloc &)
        {
            std::terminate();
        }
        catch (const std::exception &)
        {
            static_cast<LinkedSaveCheckpoint *>(context)->result = E_FAIL;
        }
    }
};
struct StorageCheckpoint
{
    StorageCheckpoint() = default;
    StorageCheckpoint(const StorageCheckpoint &) = delete;
    StorageCheckpoint &operator=(const StorageCheckpoint &) = delete;
    AtomicWriteCheckpoint phase{};
    unsigned calls{};
    HRESULT result = E_UNEXPECTED;
    wil::unique_handle lock;
    static void Observe(AtomicWriteCheckpoint phase, const std::filesystem::path &path, void *context) noexcept
    {
        auto &self = *static_cast<StorageCheckpoint *>(context);
        if (phase != self.phase)
            return;
        ++self.calls;
        SetAtomicWriteHookForTest(nullptr, nullptr);
        if (phase == AtomicWriteCheckpoint::Staged)
            self.result = WriteBytesAtomic(path, "external during staging");
        else
        {
            // An unrelated writer may deny content reads immediately after publication.
            self.lock.reset(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, nullptr));
            self.result = self.lock ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
    }
};
inline void RegisterStorageSafetyTests(const std::filesystem::path &root,
                                       const std::function<void(std::string_view, const std::function<void()> &)> &test)
{
    test(
        "Storage.HardlinkSavePreservesAliasesAndProtectsBeforeMutation",
        [&]
        {
            const auto directory = root / L"hardlink-success";
            std::filesystem::create_directories(directory);
            const auto path = directory / L"notes.md", alias = directory / L"same-notes.md";
            FileStamp original;
            RequireStorage(SUCCEEDED(SaveDocument(path, L"original\r\n", true, nullptr, original)), "seed linked document");
            RequireStorage(CreateHardLinkW(alias.c_str(), path.c_str(), nullptr), "create owned same-volume hardlink");
            const SaveRecoveryContext recovery{directory / L"state", {9, 2}, 42};
            LinkedSaveCheckpoint checkpoint{recovery.dataDirectory};
            SetAtomicWriteHookForTest(LinkedSaveCheckpoint::Observe, &checkpoint);
            const auto clear = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
            FileStamp saved;
            RequireStorage(SUCCEEDED(SaveDocument(path, L"Revised 世界 😀\r\n", true, &original, saved, false, nullptr, &recovery)),
                           "save protected hardlinked document");
            RequireStorage(checkpoint.protectedCalls == 1 && SUCCEEDED(checkpoint.result) &&
                               checkpoint.observedBytes == "\xEF\xBB\xBF"
                                                           "original\r\n" &&
                               checkpoint.snapshots.size() == 1 && checkpoint.snapshots[0].text == L"Revised 世界 😀\r\n" &&
                               checkpoint.snapshots[0].originalPath == path && checkpoint.snapshots[0].bom &&
                               checkpoint.snapshots[0].selection == recovery.selection && checkpoint.snapshots[0].generation == recovery.generation,
                           "complete immutable recovery is durable while every original byte still exists");
            StoredDocument actual, shared;
            RequireStorage(SUCCEEDED(LoadDocument(path, actual)) && SUCCEEDED(LoadDocument(alias, shared)) && actual.text == L"Revised 世界 😀\r\n" &&
                               actual.bom && actual.stamp == shared.stamp && actual.stamp == saved && actual.stamp.volume == original.volume &&
                               actual.stamp.fileIndex == original.fileIndex && actual.stamp.fileIndexHigh == original.fileIndexHigh,
                           "all aliases retain the same full native identity, BOM, EOLs and admitted source");
            RequireStorage(ProtectedSaveSnapshots(recovery.dataDirectory).empty(), "successful flushed save retires only its independent checkpoint");
        });
    test(
        "Storage.HardlinkFailuresRetainCompleteRecoveryAndBaseline",
        [&]
        {
            for (const auto fault : {AtomicWriteFailure::InPlaceWrite, AtomicWriteFailure::InPlacePartialWrite, AtomicWriteFailure::InPlaceFlush})
            {
                const auto directory = root / (L"hardlink-fault-" + std::to_wstring(static_cast<unsigned>(fault)));
                std::filesystem::create_directories(directory);
                const auto path = directory / L"notes.md", alias = directory / L"same-notes.md";
                FileStamp original;
                RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)) &&
                                   CreateHardLinkW(alias.c_str(), path.c_str(), nullptr),
                               "seed linked write-fault fixture");
                const SaveRecoveryContext recovery{directory / L"state", {3, 1}, 73};
                SetAtomicWriteFailureForTest(fault);
                const auto clear = wil::scope_exit([] { SetAtomicWriteFailureForTest(AtomicWriteFailure::None); });
                FileStamp saved = original;
                const auto result = SaveDocument(path, L"complete admitted replacement\r\n世界", true, &original, saved, false, nullptr, &recovery);
                RequireStorage(FAILED(result) && saved == original, "failed in-place write never publishes the caller's saved baseline");
                SetAtomicWriteFailureForTest(AtomicWriteFailure::None);
                const auto snapshots = ProtectedSaveSnapshots(recovery.dataDirectory);
                RequireStorage(snapshots.size() == 1 && snapshots[0].text == L"complete admitted replacement\r\n世界" && snapshots[0].bom &&
                                   snapshots[0].originalPath == path && snapshots[0].selection == recovery.selection && snapshots[0].generation == 73,
                               "each failure retains all admitted source and recovery metadata, even after a partial target write");
                std::string primary, secondary;
                FileStamp a, b;
                RequireStorage(SUCCEEDED(ReadBytes(path, primary, &a)) && SUCCEEDED(ReadBytes(alias, secondary, &b)) && primary == secondary &&
                                   a.fileIndex == original.fileIndex && a.fileIndexHigh == original.fileIndexHigh && a.volume == original.volume &&
                                   a == b,
                               "failure preserves the shared object across every alias");
                if (fault == AtomicWriteFailure::InPlaceWrite)
                    RequireStorage(primary == "original", "pre-write failure leaves original bytes intact");
                RecoveryService restart;
                RequireStorage(SUCCEEDED(restart.Initialize(recovery.dataDirectory)) && SUCCEEDED(restart.WaitForSettlement(5000)) && restart.Ready(),
                               "restart completes recovery scan");
                const auto candidates = restart.TakeCandidates();
                RequireStorage(candidates.size() == 1 && candidates[0].snapshot.text == snapshots[0].text && candidates[0].snapshot.generation == 73,
                               "ordinary restart offers the exact independent admitted version");
            }
        });
    test("Recovery.SaveProtectionSurvivesOrdinaryCoalescingAndRetirement",
         [&]
         {
             const auto directory = root / L"independent-save-protection";
             const RecoverySnapshot admitted{L"admitted source", root / L"notes.md", true, {8, 2}, 80};
             {
                 SaveRecoveryProtection protection;
                 RequireStorage(SUCCEEDED(protection.Admit(directory, admitted)), "admit independently leased save checkpoint");
                 RecoveryService writer;
                 RequireStorage(SUCCEEDED(writer.Initialize(directory)) && SUCCEEDED(writer.WaitForSettlement(5000)) && writer.Ready(),
                                "start ordinary recovery");
                 RequireStorage(writer.TakeCandidates().empty(), "active save lease is never offered as an abandoned session");
                 RequireStorage(SUCCEEDED(writer.Submit(RecoverySnapshot{L"newer edits", {}, false, {0, 2}, 81}, true)) &&
                                    SUCCEEDED(writer.WaitForSettlement(5000)) && SUCCEEDED(writer.Submit(std::nullopt, true)) &&
                                    SUCCEEDED(writer.WaitForSettlement(5000)),
                                "ordinary newer checkpoint and clean retirement complete");
                 writer.Close();
                 const auto snapshots = ProtectedSaveSnapshots(directory);
                 RequireStorage(snapshots.size() == 1 && snapshots[0].text == admitted.text && snapshots[0].generation == 80,
                                "coalescing and retiring the ordinary session never overwrites or deletes admitted save protection");
             }
             RecoveryService restart;
             RequireStorage(SUCCEEDED(restart.Initialize(directory)) && SUCCEEDED(restart.WaitForSettlement(5000)),
                            "scan released protection after interruption");
             const auto candidates = restart.TakeCandidates();
             RequireStorage(candidates.size() == 1 && candidates[0].snapshot.text == admitted.text &&
                                candidates[0].snapshot.selection == admitted.selection,
                            "guard destruction retains a complete, discoverable recovery copy");
         });
    test("Storage.HardlinkAdmissionFailureNeverMutatesTarget",
         [&]
         {
             const auto directory = root / L"hardlink-admission";
             std::filesystem::create_directories(directory);
             const auto path = directory / L"notes.md", alias = directory / L"same.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"retained", false, nullptr, original)) &&
                                CreateHardLinkW(alias.c_str(), path.c_str(), nullptr),
                            "seed admission target");
             FileStamp saved = original;
             RequireStorage(FAILED(SaveDocument(path, L"new source", false, &original, saved)), "hardlink save without recovery context is declined");
             const auto blocked = directory / L"blocked-state";
             RequireStorage(SUCCEEDED(WriteBytesAtomic(blocked, "a file blocks the recovery directory")), "seed unavailable recovery location");
             const SaveRecoveryContext unavailable{blocked, {0, 0}, 1}, invalid{directory / L"state", {1000, 0}, 2};
             RequireStorage(FAILED(SaveDocument(path, L"new source", false, &original, saved, false, nullptr, &unavailable)) &&
                                FAILED(SaveDocument(path, L"new source", false, &original, saved, false, nullptr, &invalid)),
                            "unavailable or invalid recovery fails admission");
             const auto fullDirectory = directory / L"full-state";
             std::filesystem::create_directories(fullDirectory / L"Recovery");
             for (unsigned index = 0; index < 512; ++index)
             {
                 const auto leaf = fullDirectory / L"Recovery" / (L"unrelated-" + std::to_wstring(index));
                 wil::unique_handle file(CreateFileW(leaf.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
                 RequireStorage(static_cast<bool>(file), "seed the bounded recovery-directory admission fixture");
             }
             const SaveRecoveryContext full{fullDirectory, {0, 0}, 3};
             RequireStorage(SaveDocument(path, L"new source", false, &original, saved, false, nullptr, &full) ==
                                HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES),
                            "full recovery-directory budget declines before target mutation");
             StoredDocument actual, shared;
             RequireStorage(saved == original && SUCCEEDED(LoadDocument(path, actual)) && SUCCEEDED(LoadDocument(alias, shared)) &&
                                actual.text == L"retained" && actual.stamp == original && shared.stamp == original,
                            "every admission failure preserves original bytes, all identities and caller baseline");
         });
    test("Storage.HardlinkSaveSuccessSurvivesBlockedRecoveryRetirement",
         [&]
         {
             const auto directory = root / L"hardlink-retirement";
             std::filesystem::create_directories(directory);
             const auto path = directory / L"notes.md", alias = directory / L"same.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)) &&
                                CreateHardLinkW(alias.c_str(), path.c_str(), nullptr),
                            "seed retirement fixture");
             const SaveRecoveryContext recovery{directory / L"state", {0, 4}, 101};
             struct Inspection
             {
                 explicit Inspection(std::filesystem::path input) : directory(std::move(input)) {}
                 Inspection(const Inspection &) = delete;
                 Inspection &operator=(const Inspection &) = delete;
                 std::filesystem::path directory;
                 wil::unique_handle record;
                 HRESULT result = E_UNEXPECTED;
             } inspection{recovery.dataDirectory};
             SetAtomicWriteHookForTest(
                 [](AtomicWriteCheckpoint phase, const std::filesystem::path &, void *context) noexcept
                 {
                     if (phase != AtomicWriteCheckpoint::InPlaceProtected)
                         return;
                     auto &current = *static_cast<Inspection *>(context);
                     // A test-only reader keeps the complete protection file but
                     // denies its deletion. The production scanner omits live leases.
                     try
                     {
                         for (const auto &entry : std::filesystem::directory_iterator(current.directory / L"Recovery"))
                             if (entry.path().extension() == L".rtd")
                                 current.record.reset(CreateFileW(entry.path().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                                                  FILE_ATTRIBUTE_NORMAL, nullptr));
                         current.result = current.record ? S_OK : E_FAIL;
                     }
                     catch (const std::bad_alloc &)
                     {
                         std::terminate();
                     }
                     catch (const std::exception &)
                     {
                         current.result = E_FAIL; // Test-hook ABI reports inspection failure.
                     }
                 },
                 &inspection);
             const auto clear = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
             FileStamp saved;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"fully saved", false, &original, saved, false, nullptr, &recovery)) &&
                                SUCCEEDED(inspection.result),
                            "recovery retirement failure cannot turn completed storage into a failed save");
             inspection.record.reset();
             StoredDocument actual, shared;
             RequireStorage(SUCCEEDED(LoadDocument(path, actual)) && SUCCEEDED(LoadDocument(alias, shared)) && actual.text == L"fully saved" &&
                                actual.stamp == shared.stamp && actual.stamp == saved,
                            "successful known-byte stamp remains the new baseline");
             const auto snapshots = ProtectedSaveSnapshots(recovery.dataDirectory);
             RequireStorage(snapshots.size() == 1 && snapshots[0].text == actual.text && snapshots[0].generation == 101,
                            "blocked retirement retains an extra valid recovery copy");
         });
    test("Storage.NewHardlinkDuringStagingSelectsProtectedInPlaceSave",
         [&]
         {
             const auto directory = root / L"late-hardlink";
             std::filesystem::create_directories(directory);
             const auto path = directory / L"notes.md", alias = directory / L"late-alias.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)), "seed late-link target");
             const SaveRecoveryContext recovery{directory / L"state", {0, 3}, 94};
             LinkedSaveCheckpoint checkpoint{recovery.dataDirectory, alias};
             SetAtomicWriteHookForTest(LinkedSaveCheckpoint::Observe, &checkpoint);
             const auto clear = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
             FileStamp saved;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"new admitted source", false, &original, saved, false, nullptr, &recovery)) &&
                                checkpoint.stagedCalls == 1 && checkpoint.protectedCalls == 1 && SUCCEEDED(checkpoint.result),
                            "final staging check detects a new alias and reselects protected in-place writing");
             StoredDocument actual, shared;
             RequireStorage(SUCCEEDED(LoadDocument(path, actual)) && SUCCEEDED(LoadDocument(alias, shared)) &&
                                actual.text == L"new admitted source" && actual.stamp == shared.stamp && saved == actual.stamp &&
                                actual.stamp.fileIndex == original.fileIndex && actual.stamp.fileIndexHigh == original.fileIndexHigh,
                            "late-created alias receives the same admitted bytes without splitting the object");
         });
    test("Storage.SymbolicSavePreservesEntryAndResolvesFinalTarget",
         [&]
         {
             const auto directory = root / L"symbolic-save";
             std::filesystem::create_directories(directory);
             const auto target = directory / L"世界.md", link = directory / L"linked-notes.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(target, L"original", false, nullptr, original)), "seed symlink target");
             const auto created = CreateSymbolicLinkW(link.c_str(), target.filename().c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
             const auto linkError = GetLastError();
             RequireStorage(created, std::format("create owned relative symlink (Win32 {})", linkError).c_str());
             const auto removeLink = wil::scope_exit([&] { (void)DeleteFileW(link.c_str()); });
             FileStamp saved;
             RequireStorage(SUCCEEDED(SaveDocument(link, L"revised target\r\n", true, &original, saved)), "save final symlink target atomically");
             const auto attributes = GetFileAttributesW(link.c_str());
             StoredDocument actual, followed;
             RequireStorage(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                                SUCCEEDED(LoadDocument(target, actual)) && SUCCEEDED(LoadDocument(link, followed)) &&
                                actual.text == L"revised target\r\n" && actual.bom && actual.stamp == followed.stamp && actual.stamp == saved &&
                                (actual.stamp.fileIndex != original.fileIndex || actual.stamp.fileIndexHigh != original.fileIndexHigh),
                            "relative Unicode symlink remains intact while its target receives atomic replacement");
             const auto alias = directory / L"hardlinked-target.md";
             RequireStorage(CreateHardLinkW(alias.c_str(), target.c_str(), nullptr), "add hardlink to symlink target");
             const SaveRecoveryContext recovery{directory / L"state", {0, 4}, 111};
             const auto prior = saved;
             RequireStorage(SUCCEEDED(SaveDocument(link, L"shared target", false, &prior, saved, false, nullptr, &recovery)),
                            "symlink to hardlinked target selects protected save");
             RequireStorage(SUCCEEDED(LoadDocument(alias, actual)) && actual.text == L"shared target" && actual.stamp == saved &&
                                actual.stamp.fileIndex == prior.fileIndex && actual.stamp.fileIndexHigh == prior.fileIndexHigh &&
                                (GetFileAttributesW(link.c_str()) & FILE_ATTRIBUTE_REPARSE_POINT),
                            "symbolic entry and hardlink target identity both survive");
         });
    test("Storage.StampMatchesFullNativeFileIdentity",
         [&]
         {
             const auto path = root / L"native-file-identity.md";
             FileStamp saved;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"Invented native identity fixture", false, nullptr, saved)),
                            "seed native identity fixture");
             wil::unique_handle file(CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                                 FILE_ATTRIBUTE_NORMAL, nullptr));
             FILE_ID_INFO info{};
             RequireStorage(file && GetFileInformationByHandleEx(file.get(), FileIdInfo, &info, sizeof(info)),
                            "query the actual filesystem's full file ID");
             std::uint64_t low{}, high{};
             std::memcpy(&low, info.FileId.Identifier, sizeof(low));
             std::memcpy(&high, info.FileId.Identifier + sizeof(low), sizeof(high));
             RequireStorage(saved.volume == info.VolumeSerialNumber && saved.fileIndex == low && saved.fileIndexHigh == high,
                            "saved stamp retains the full 64-bit volume serial and 128-bit filesystem identifier");
         });
    test("Storage.UnsupportedFilesReportKnownUnitCount",
         [&]
         {
             const auto path = root / L"unsupported-units.md";
             std::string bytes;
             bytes.reserve(4u * 32769u);
             for (unsigned index = 0; index < 32769u; ++index)
                 bytes += "\xF0\x9F\x98\x80";
             RequireStorage(SUCCEEDED(WriteBytesAtomic(path, bytes)), "seed bounded UTF-8 whose decoded size exceeds capacity");
             StoredDocument document{L"Retained source", true, {}};
             std::size_t units{};
             RequireStorage(LoadDocument(path, document, &units) == HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE) && units == 65538u &&
                                document.text == L"Retained source" && document.bom,
                            "capacity error reports exact UTF-16 units without changing the current document");
             RequireStorage(SUCCEEDED(WriteBytesAtomic(path, "\xFF\xFEtext")), "seed UTF-16 bytes in an unsupported file");
             RequireStorage(LoadDocument(path, document, &units) == HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) && units == 0 &&
                                document.text == L"Retained source" && document.bom,
                            "unsupported encoding remains a specific failure with no invented character count");
         });
    test("Storage.TimestampOnlyTouchDoesNotConflict",
         [&]
         {
             const auto path = root / L"timestamp-touch.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"same source", false, nullptr, original)), "seed timestamp-only target");
             {
                 wil::unique_handle file(CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
                 const auto later = original.modified + 100000000u;
                 const FILETIME modified{static_cast<DWORD>(later), static_cast<DWORD>(later >> 32u)};
                 RequireStorage(file && SetFileTime(file.get(), nullptr, nullptr, &modified), "touch the same file without changing bytes");
             }
             FileStamp touched;
             std::string bytes;
             RequireStorage(SUCCEEDED(ReadBytes(path, bytes, &touched)) && touched.modified != original.modified && touched.hash == original.hash,
                            "the touch really changed only metadata on the same object");
             FileStamp saved;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"new source", false, &original, saved)), "timestamp-only touch does not ask to overwrite");
         });
    test("Storage.SharedWriterReadAndSaveAsBaseline",
         [&]
         {
             const auto path = root / L"shared-writer.md";
             const auto alias = root / L"shared-alias.md";
             const auto copy = root / L"explicit-other-target.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)), "seed shared-reader target");
             {
                 wil::unique_handle writer(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
                 StoredDocument actual;
                 RequireStorage(writer && SUCCEEDED(LoadDocument(path, actual)) && actual.text == L"original" && actual.stamp == original,
                                "a cooperative writer does not prevent a stable content read");
             }
             RequireStorage(SUCCEEDED(WriteBytesAtomic(path, "external")), "replace current target before Save As");
             RequireStorage(CreateHardLinkW(alias.c_str(), path.c_str(), nullptr), "create an owned alias of the current external object");
             FileStamp saved = original;
             RequireStorage(SaveDocument(alias, L"mine", false, &original, saved, true, &path) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) &&
                                saved == original,
                            "Save As through an alias of a replaced current target still compares the original saved baseline");
             RequireStorage(SUCCEEDED(WriteBytesAtomic(copy, "other document")), "seed the explicitly chosen different destination");
             RequireStorage(SUCCEEDED(SaveDocument(copy, L"mine", false, &original, saved, true, &path)),
                            "Save As to an explicitly approved different object does not inherit the original target's conflict");
             StoredDocument source, destination;
             RequireStorage(SUCCEEDED(LoadDocument(path, source)) && source.text == L"external" && SUCCEEDED(LoadDocument(copy, destination)) &&
                                destination.text == L"mine" && destination.stamp == saved,
                            "the original external bytes remain while the separate copy publishes its own stamp");
         });
    test("Storage.RechecksIdentityAfterStaging",
         [&]
         {
             const auto path = root / L"stage-race.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)), "seed staging-race target");
             StorageCheckpoint checkpoint;
             checkpoint.phase = AtomicWriteCheckpoint::Staged;
             SetAtomicWriteHookForTest(StorageCheckpoint::Observe, &checkpoint);
             const auto clear = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
             FileStamp saved = original;
             const auto result = SaveDocument(path, L"mine", false, &original, saved);
             RequireStorage(checkpoint.calls == 1 && SUCCEEDED(checkpoint.result), "external edit occurs after staging");
             StoredDocument actual;
             RequireStorage(SUCCEEDED(LoadDocument(path, actual)), "read target after staging race");
             RequireStorage(result == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && actual.text == L"external during staging" && saved == original,
                            "late external edit is preserved and unsuccessful save does not publish its stamp");
             for (const auto &entry : std::filesystem::directory_iterator(root))
                 RequireStorage(entry.path().extension() != L".tmp", "late conflict removes only its owned staging file");
         });
    test("Storage.PublishedSaveDoesNotDependOnContentReadback",
         [&]
         {
             const auto path = root / L"published-save.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"original", false, nullptr, original)), "seed publication target");
             StorageCheckpoint checkpoint;
             checkpoint.phase = AtomicWriteCheckpoint::Published;
             SetAtomicWriteHookForTest(StorageCheckpoint::Observe, &checkpoint);
             const auto clear = wil::scope_exit([] { SetAtomicWriteHookForTest(nullptr, nullptr); });
             FileStamp saved;
             const auto result = SaveDocument(path, L"published", false, &original, saved);
             RequireStorage(checkpoint.calls == 1 && SUCCEEDED(checkpoint.result), "external handle denies content read after publication");
             checkpoint.lock.reset();
             StoredDocument actual;
             RequireStorage(SUCCEEDED(LoadDocument(path, actual)) && actual.text == L"published", "the admitted write really was published");
             RequireStorage(SUCCEEDED(result) && saved == actual.stamp,
                            "published save returns its exact owned-write stamp without reopening content");
             FileStamp next;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"next", false, &saved, next)), "next save accepts the successful preceding baseline");
         });
    test("Storage.ReplacementIdentityDetectsEqualBytesAndTimestamp",
         [&]
         {
             const auto path = root / L"replaced-identity.md";
             const auto replacement = root / L"other-identity.md";
             FileStamp original;
             RequireStorage(SUCCEEDED(SaveDocument(path, L"same", false, nullptr, original)), "seed identity target");
             RequireStorage(SUCCEEDED(WriteBytesAtomic(replacement, "same")), "seed a different file object with equal bytes");
             {
                 wil::unique_handle file(CreateFileW(replacement.c_str(), FILE_WRITE_ATTRIBUTES,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                                     FILE_ATTRIBUTE_NORMAL, nullptr));
                 const FILETIME modified{static_cast<DWORD>(original.modified), static_cast<DWORD>(original.modified >> 32u)};
                 RequireStorage(file && SetFileTime(file.get(), nullptr, nullptr, &modified), "give the replacement the exact saved timestamp");
             }
             RequireStorage(MoveFileExW(replacement.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH),
                            "replace the target object");
             FileStamp saved = original;
             RequireStorage(SaveDocument(path, L"mine", false, &original, saved) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH) && saved == original,
                            "a different target object requires a conflict decision even with matching bytes, size and timestamp");
             StoredDocument actual;
             RequireStorage(SUCCEEDED(LoadDocument(path, actual)) && actual.text == L"same", "replacement bytes remain unchanged");
         });
}
} // namespace RedTilde::Testing
