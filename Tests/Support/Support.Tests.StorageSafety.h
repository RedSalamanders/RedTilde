#pragma once
#include "Common/Storage.h"
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
