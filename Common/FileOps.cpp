#include "FileOps.h"
#include "Files.h"
#include "MonitorDiagnostics.h"
#include "ProcessWork.h"
#include "Storage.h"
#include <DxUi/Failure.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <format>
#include <mutex>
#include <shellapi.h>
#include <shlobj.h>
#include <wil/resource.h>

namespace RedTilde
{
namespace
{
constexpr std::size_t MaximumClipboardPaths   = 64u;
constexpr wchar_t PreferredDropEffectFormat[] = L"Preferred DropEffect";

bool EqualOrdinalIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
{
    return left.size() == right.size() &&
           (left.empty() || CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL);
}

bool IsRegularNonReparseEntry(const std::filesystem::path& path, bool expectDirectory, bool* outReparse = nullptr) noexcept
{
    if (outReparse)
        *outReparse = false;
    if (path.empty() || ! path.is_absolute())
        return false;
    const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
    const wil::unique_handle h(
        CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, flags, nullptr));
    if (! h)
        return false;
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (! GetFileInformationByHandleEx(h.get(), FileAttributeTagInfo, &info, sizeof(info)))
        return false;
    if ((info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(info.ReparseTag) != FALSE)
    {
        if (outReparse)
            *outReparse = true;
        return false;
    }
    const bool isDir = (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
    return isDir == expectDirectory;
}

bool HasAnyReparseAncestorOrSelf(const std::filesystem::path& path) noexcept
{
    if (path.empty() || ! path.is_absolute())
        return true;
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        const wil::unique_handle h(CreateFileW(path.c_str(),
                                               FILE_READ_ATTRIBUTES,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr,
                                               OPEN_EXISTING,
                                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                               nullptr));
        if (h)
        {
            FILE_ATTRIBUTE_TAG_INFO info{};
            if (GetFileInformationByHandleEx(h.get(), FileAttributeTagInfo, &info, sizeof(info)) &&
                (info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(info.ReparseTag) != FALSE)
                return true;
        }
    }
    for (auto current = path.parent_path(); ! current.empty() && current != current.root_path(); current = current.parent_path())
    {
        const wil::unique_handle h(CreateFileW(current.c_str(),
                                               FILE_READ_ATTRIBUTES,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr,
                                               OPEN_EXISTING,
                                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                               nullptr));
        if (! h)
            return true;
        FILE_ATTRIBUTE_TAG_INFO info{};
        if (! GetFileInformationByHandleEx(h.get(), FileAttributeTagInfo, &info, sizeof(info)))
            return true;
        if ((info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0u && IsReparseTagNameSurrogate(info.ReparseTag) != FALSE)
            return true;
    }
    return false;
}

bool IsReservedDosName(std::wstring_view leaf) noexcept
{
    const auto dot  = leaf.find(L'.');
    const auto stem = dot == std::wstring_view::npos ? leaf : leaf.substr(0, dot);
    static constexpr std::array<std::wstring_view, 22> Reserved{L"CON",  L"PRN",  L"AUX",  L"NUL",  L"COM1", L"COM2", L"COM3", L"COM4",
                                                                L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1", L"LPT2", L"LPT3",
                                                                L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"};
    return std::ranges::any_of(Reserved, [&](std::wstring_view candidate) { return EqualOrdinalIgnoreCase(stem, candidate); });
}

std::optional<std::filesystem::path> TryAdmitExecutablePath(const std::filesystem::path& candidate) noexcept
{
    if (candidate.empty() || ! candidate.is_absolute())
        return std::nullopt;
    const auto norm = candidate.lexically_normal();
    if (! EqualOrdinalIgnoreCase(norm.extension().native(), L".exe"))
        return std::nullopt;
    if (! IsRegularNonReparseEntry(norm, false))
        return std::nullopt;
    return norm;
}

std::optional<std::filesystem::path> ReadAppPathsRegistry(HKEY rootKey) noexcept
{
    std::array<wchar_t, 32768> buffer{};
    DWORD bytes          = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
    DWORD type           = 0u;
    const LSTATUS status = RegGetValueW(rootKey,
                                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\RedSalamander.exe",
                                        nullptr,
                                        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ,
                                        &type,
                                        buffer.data(),
                                        &bytes);
    if (status != ERROR_SUCCESS || bytes < sizeof(wchar_t))
        return std::nullopt;
    buffer.back() = L'\0';
    return TryAdmitExecutablePath(std::filesystem::path(buffer.data()));
}
} // namespace

std::wstring QuoteWindowsCliArgument(std::wstring_view argument)
{
    if (argument.empty())
        return L"\"\"";
    const bool needsQuotes = argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (! needsQuotes)
        return std::wstring(argument);
    std::wstring out;
    out.push_back(L'"');
    std::size_t backslashes{};
    for (const wchar_t ch : argument)
    {
        if (ch == L'\\')
        {
            ++backslashes;
        }
        else if (ch == L'"')
        {
            out.append(backslashes * 2u + 1u, L'\\');
            out.push_back(L'"');
            backslashes = 0u;
        }
        else
        {
            out.append(backslashes, L'\\');
            out.push_back(ch);
            backslashes = 0u;
        }
    }
    out.append(backslashes * 2u, L'\\');
    out.push_back(L'"');
    return out;
}

std::optional<std::filesystem::path> WindowsShellReveal::FindRedSalamander(const std::optional<std::filesystem::path>& overridePath) const noexcept
try
{
    if (overridePath)
        if (auto admitted = TryAdmitExecutablePath(*overridePath))
            return admitted;
    if (auto hkcu = ReadAppPathsRegistry(HKEY_CURRENT_USER))
        return hkcu;
    if (auto hklm = ReadAppPathsRegistry(HKEY_LOCAL_MACHINE))
        return hklm;
    wil::unique_cotaskmem_string localAppData;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData)) && localAppData)
    {
        const std::filesystem::path base(localAppData.get());
        if (auto perUser = TryAdmitExecutablePath(base / L"Programs" / L"RedSalamander" / L"RedSalamander.exe"))
            return perUser;
        if (auto launcher = TryAdmitExecutablePath(base / L"Programs" / L"RedSalamander" / L"RedLauncher.exe"))
            return launcher;
    }
    wil::unique_cotaskmem_string programFiles;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &programFiles)) && programFiles)
    {
        if (auto pf = TryAdmitExecutablePath(std::filesystem::path(programFiles.get()) / L"RedSalamander" / L"RedSalamander.exe"))
            return pf;
    }
    return std::nullopt;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsShellReveal::RevealInExplorer(const std::filesystem::path& targetPath) noexcept
try
{
    if (targetPath.empty() || ! targetPath.is_absolute() || HasAnyReparseAncestorOrSelf(targetPath))
        return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);

    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool comOwned  = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
        return initHr;
    const auto uninit = wil::scope_exit([&]() noexcept
    {
        if (comOwned)
            CoUninitialize();
    });

    const auto norm   = targetPath.lexically_normal();
    const DWORD attrs = GetFileAttributesW(norm.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());

    const wil::unique_any<LPITEMIDLIST, decltype(&CoTaskMemFree), CoTaskMemFree> itemId(ILCreateFromPathW(norm.c_str()));
    if (itemId)
    {
        const HRESULT selHr = SHOpenFolderAndSelectItems(itemId.get(), 0, nullptr, 0);
        if (SUCCEEDED(selHr))
            return S_OK;
    }

    const auto folder = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0u ? norm : norm.parent_path();
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask  = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"explore";
    sei.lpFile = folder.c_str();
    sei.nShow  = SW_SHOWNORMAL;
    if (! ShellExecuteExW(&sei))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsShellReveal::RevealInRedSalamander(const std::filesystem::path& targetPath, const std::optional<std::filesystem::path>& overridePath) noexcept
try
{
    if (targetPath.empty() || ! targetPath.is_absolute() || HasAnyReparseAncestorOrSelf(targetPath))
        return HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    const auto exe = FindRedSalamander(overridePath);
    if (! exe)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    const auto normTarget = targetPath.lexically_normal();
    const auto arg        = std::wstring(L"--select=") + normTarget.native();
    std::wstring cmdLine  = QuoteWindowsCliArgument(exe->native()) + L" " + QuoteWindowsCliArgument(arg);
    const auto workDir    = exe->parent_path();

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (! CreateProcessW(exe->c_str(), cmdLine.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, workDir.c_str(), &si, &pi))
    {
        const DWORD err = GetLastError();
        return HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    wil::unique_handle proc(pi.hProcess);
    wil::unique_handle thread(pi.hThread);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

FileNameValidationError WindowsWorkspaceFileOps::ValidateFileName(std::wstring_view candidate) const noexcept
{
    if (candidate.empty())
        return FileNameValidationError::Empty;
    if (candidate.size() > 255u || ! DxUi::RichText::IsValidUtf16(candidate))
        return FileNameValidationError::TooLong;
    if (candidate == L"." || candidate == L"..")
        return FileNameValidationError::ReservedName;
    for (const wchar_t ch : candidate)
    {
        if (ch < L' ' || ch == 0x7Fu)
            return FileNameValidationError::InvalidCharacters;
        if (ch == L'\\' || ch == L'/')
            return FileNameValidationError::PathSeparator;
        if (ch == L'<' || ch == L'>' || ch == L':' || ch == L'"' || ch == L'|' || ch == L'?' || ch == L'*')
            return FileNameValidationError::InvalidCharacters;
    }
    if (candidate.back() == L'.' || candidate.back() == L' ')
        return FileNameValidationError::TrailingDotOrSpace;
    if (IsReservedDosName(candidate))
        return FileNameValidationError::ReservedName;
    return FileNameValidationError::None;
}

HRESULT WindowsWorkspaceFileOps::RenameItem(const std::filesystem::path& sourcePath, std::wstring_view newLeafName, FileOperationOutcome& outOutcome) noexcept
try
{
    outOutcome            = {};
    outOutcome.sourcePath = sourcePath.lexically_normal();
    if (outOutcome.sourcePath.empty() || ! outOutcome.sourcePath.is_absolute())
        return outOutcome.status = E_INVALIDARG;
    if (ValidateFileName(newLeafName) != FileNameValidationError::None)
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    if (HasAnyReparseAncestorOrSelf(outOutcome.sourcePath))
    {
        outOutcome.reparseRejected = true;
        return outOutcome.status   = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    }
    const auto destination     = outOutcome.sourcePath.parent_path() / newLeafName;
    outOutcome.destinationPath = destination.lexically_normal();
    if (EqualOrdinalIgnoreCase(outOutcome.sourcePath.native(), outOutcome.destinationPath.native()) &&
        outOutcome.sourcePath.native() == outOutcome.destinationPath.native())
        return S_OK;

    const bool exactCaseOnlyRename = EqualOrdinalIgnoreCase(outOutcome.sourcePath.native(), outOutcome.destinationPath.native());
    if (! exactCaseOnlyRename && GetFileAttributesW(outOutcome.destinationPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        outOutcome.collision     = true;
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }
    if (! MoveFileExW(outOutcome.sourcePath.c_str(), outOutcome.destinationPath.c_str(), exactCaseOnlyRename ? MOVEFILE_REPLACE_EXISTING : 0u))
    {
        const DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
            outOutcome.collision = true;
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsWorkspaceFileOps::PasteItem(const std::filesystem::path& sourcePath,
                                           const std::filesystem::path& destinationDirectory,
                                           DropEffectKind effect,
                                           bool replaceExisting,
                                           bool keepBothCopy,
                                           FileOperationOutcome& outOutcome) noexcept
try
{
    outOutcome            = {};
    outOutcome.sourcePath = sourcePath.lexically_normal();
    const auto normDir    = destinationDirectory.lexically_normal();
    if (outOutcome.sourcePath.empty() || ! outOutcome.sourcePath.is_absolute() || normDir.empty() || ! normDir.is_absolute())
        return outOutcome.status = E_INVALIDARG;
    if (HasAnyReparseAncestorOrSelf(outOutcome.sourcePath) || HasAnyReparseAncestorOrSelf(normDir))
    {
        outOutcome.reparseRejected = true;
        return outOutcome.status   = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    }
    const DWORD dirAttrs = GetFileAttributesW(normDir.c_str());
    if (dirAttrs == INVALID_FILE_ATTRIBUTES || (dirAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0u)
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_DIRECTORY);
    const DWORD srcAttrs = GetFileAttributesW(outOutcome.sourcePath.c_str());
    if (srcAttrs == INVALID_FILE_ATTRIBUTES)
        return outOutcome.status = HRESULT_FROM_WIN32(GetLastError());
    const bool srcIsDir = (srcAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0u;
    if (! srcIsDir && ! IsMarkdownFile(outOutcome.sourcePath) && ! EqualOrdinalIgnoreCase(outOutcome.sourcePath.extension().native(), L".txt"))
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);

    auto destination        = (normDir / outOutcome.sourcePath.filename()).lexically_normal();
    const bool sameAsSource = EqualOrdinalIgnoreCase(outOutcome.sourcePath.native(), destination.native());
    if (sameAsSource && effect == DropEffectKind::Move)
    {
        outOutcome.destinationPath = destination;
        return S_OK;
    }
    if ((sameAsSource && effect == DropEffectKind::Copy) || (keepBothCopy && GetFileAttributesW(destination.c_str()) != INVALID_FILE_ATTRIBUTES))
    {
        const auto stem = outOutcome.sourcePath.stem().native();
        const auto ext  = srcIsDir ? std::wstring{} : outOutcome.sourcePath.extension().native();
        bool foundUnique{};
        for (unsigned index = 1u; index <= 99u; ++index)
        {
            const std::wstring candidateName = index == 1u ? (stem + L" - Copy" + ext) : std::format(L"{} - Copy ({}){}", stem, index, ext);
            const auto candidate             = (normDir / candidateName).lexically_normal();
            if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                destination = candidate;
                foundUnique = true;
                break;
            }
        }
        if (! foundUnique)
            return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }
    outOutcome.destinationPath = destination;
    if (! replaceExisting && GetFileAttributesW(destination.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        outOutcome.collision     = true;
        return outOutcome.status = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }

    if (effect == DropEffectKind::Move)
    {
        if (! MoveFileExW(outOutcome.sourcePath.c_str(), destination.c_str(), (replaceExisting ? MOVEFILE_REPLACE_EXISTING : 0u) | MOVEFILE_COPY_ALLOWED))
        {
            const DWORD err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
                outOutcome.collision = true;
            return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
        }
        return S_OK;
    }

    if (srcIsDir)
    {
        // Recursive copy bounded explicitly for safety.
        std::error_code ec;
        std::filesystem::copy(outOutcome.sourcePath,
                              destination,
                              replaceExisting ? std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
                                              : std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing,
                              ec);
        if (ec)
            return outOutcome.status = HRESULT_FROM_WIN32(static_cast<DWORD>(ec.value()));
        return S_OK;
    }
    if (! CopyFileW(outOutcome.sourcePath.c_str(), destination.c_str(), replaceExisting ? FALSE : TRUE))
    {
        const DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS)
            outOutcome.collision = true;
        return outOutcome.status = HRESULT_FROM_WIN32(err != ERROR_SUCCESS ? err : ERROR_GEN_FAILURE);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WriteDropFilesToClipboard(HWND owner, const FileClipboardPayload& payload) noexcept
try
{
    if (payload.paths.empty() || payload.paths.size() > MaximumClipboardPaths)
        return E_INVALIDARG;
    std::size_t charsNeeded = 1u; // Trailing double NUL.
    std::vector<std::wstring> cleaned;
    cleaned.reserve(payload.paths.size());
    for (const auto& p : payload.paths)
    {
        const auto norm = p.lexically_normal().native();
        if (norm.empty() || ! p.is_absolute() || ! DxUi::RichText::IsValidUtf16(norm) || norm.find(L'\0') != std::wstring::npos)
            return E_INVALIDARG;
        charsNeeded += norm.size() + 1u;
        cleaned.push_back(norm);
    }

    const std::size_t hdropBytes = sizeof(DROPFILES) + charsNeeded * sizeof(wchar_t);
    wil::unique_hglobal dropMem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, hdropBytes));
    wil::unique_hglobal effectMem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DWORD)));
    if (! dropMem || ! effectMem)
        return E_OUTOFMEMORY;

    {
        void* const ptr = GlobalLock(dropMem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock = wil::scope_exit([&]() noexcept { GlobalUnlock(dropMem.get()); });
        auto* df          = static_cast<DROPFILES*>(ptr);
        df->pFiles        = sizeof(DROPFILES);
        df->fWide         = TRUE;
        auto* dst         = reinterpret_cast<wchar_t*>(static_cast<std::byte*>(ptr) + sizeof(DROPFILES));
        for (const auto& str : cleaned)
        {
            std::memcpy(dst, str.data(), str.size() * sizeof(wchar_t));
            dst += str.size();
            *dst++ = L'\0';
        }
        *dst = L'\0';
    }
    {
        void* const ptr = GlobalLock(effectMem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock         = wil::scope_exit([&]() noexcept { GlobalUnlock(effectMem.get()); });
        *static_cast<DWORD*>(ptr) = payload.effect == DropEffectKind::Move ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
    }

    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });
    (void)EmptyClipboard();
    if (! SetClipboardData(CF_HDROP, dropMem.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)dropMem.release();
    const UINT format = RegisterClipboardFormatW(PreferredDropEffectFormat);
    if (format != 0u && SetClipboardData(format, effectMem.get()))
        (void)effectMem.release();
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT ReadDropFilesFromClipboard(HWND owner, FileClipboardPayload& outPayload) noexcept
try
{
    outPayload = {};
    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });

    HANDLE hdropHandle = GetClipboardData(CF_HDROP);
    if (! hdropHandle)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    const auto* df = static_cast<const DROPFILES*>(GlobalLock(hdropHandle));
    if (! df)
        return HRESULT_FROM_WIN32(GetLastError());
    const auto unlock            = wil::scope_exit([&]() noexcept { GlobalUnlock(hdropHandle); });
    const std::size_t totalBytes = GlobalSize(hdropHandle);
    if (totalBytes < sizeof(DROPFILES) || df->pFiles > totalBytes || ! df->fWide)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    const auto* cursor          = reinterpret_cast<const wchar_t*>(reinterpret_cast<const std::byte*>(df) + df->pFiles);
    const std::size_t maxWchars = (totalBytes - df->pFiles) / sizeof(wchar_t);
    std::size_t pos{};
    while (pos < maxWchars && cursor[pos] != L'\0' && outPayload.paths.size() < MaximumClipboardPaths)
    {
        const std::size_t len = wcsnlen(cursor + pos, maxWchars - pos);
        if (len == 0u || pos + len >= maxWchars)
            break;
        const std::wstring_view raw(cursor + pos, len);
        if (DxUi::RichText::IsValidUtf16(raw))
        {
            std::filesystem::path candidate(raw);
            if (candidate.is_absolute())
                outPayload.paths.push_back(candidate.lexically_normal());
        }
        pos += len + 1u;
    }
    if (outPayload.paths.empty())
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    outPayload.effect = DropEffectKind::Copy;
    const UINT format = RegisterClipboardFormatW(PreferredDropEffectFormat);
    if (format != 0u)
    {
        if (HANDLE hEffect = GetClipboardData(format))
        {
            if (const auto* val = static_cast<const DWORD*>(GlobalLock(hEffect)))
            {
                const auto unlockEffect = wil::scope_exit([&]() noexcept { GlobalUnlock(hEffect); });
                if (GlobalSize(hEffect) >= sizeof(DWORD) && ((*val & DROPEFFECT_MOVE) != 0u))
                    outPayload.effect = DropEffectKind::Move;
            }
        }
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WriteUnicodeTextToClipboard(HWND owner, std::wstring_view text) noexcept
try
{
    if (text.size() > 65536u || ! DxUi::RichText::IsValidUtf16(text))
        return E_INVALIDARG;
    wil::unique_hglobal mem(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (text.size() + 1u) * sizeof(wchar_t)));
    if (! mem)
        return E_OUTOFMEMORY;
    {
        void* const ptr = GlobalLock(mem.get());
        if (! ptr)
            return HRESULT_FROM_WIN32(GetLastError());
        const auto unlock = wil::scope_exit([&]() noexcept { GlobalUnlock(mem.get()); });
        if (! text.empty())
            std::memcpy(ptr, text.data(), text.size() * sizeof(wchar_t));
        static_cast<wchar_t*>(ptr)[text.size()] = L'\0';
    }
    if (! OpenClipboard(owner))
        return HRESULT_FROM_WIN32(GetLastError());
    const auto closeCb = wil::scope_exit([]() noexcept { (void)CloseClipboard(); });
    (void)EmptyClipboard();
    if (! SetClipboardData(CF_UNICODETEXT, mem.get()))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)mem.release();
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

#pragma warning(push)
#pragma warning(disable : 4625 4626 5026 5027)
struct FileOpsService::State
{
    std::mutex mutex;
    wil::unique_event wake;
    HWND window{};
    UINT message{};
    std::shared_ptr<IShellReveal> reveal;
    std::shared_ptr<IWorkspaceFileOps> fileOps;
    std::deque<WorkspaceFileOpRequest> queue;
    std::deque<WorkspaceFileOpResult> completed;
    bool busy{};
    bool closing{};

    static void CALLBACK Worker(PTP_CALLBACK_INSTANCE instance, void* context) noexcept
    {
        (void)CallbackMayRunLong(instance);
        ProcessWorkLease lease = std::move(static_cast<ProcessWorkItem<State>*>(context)->lease);
        const std::unique_ptr<ProcessWorkItem<State>> ownership(static_cast<ProcessWorkItem<State>*>(context));
        const auto& self = ownership->state;
        try
        {
            const DxUi::Failure::Scope failureScope;
            for (;;)
            {
                WorkspaceFileOpRequest req;
                std::shared_ptr<IShellReveal> reveal;
                std::shared_ptr<IWorkspaceFileOps> ops;
                {
                    std::lock_guard lock(self->mutex);
                    if (self->closing)
                        return;
                    if (self->queue.empty())
                    {
                        self->busy = false;
                        // Wait on wake event; avoid spinning.
                        goto waitNext;
                    }
                    req = std::move(self->queue.front());
                    self->queue.pop_front();
                    self->busy = true;
                    reveal     = self->reveal;
                    ops        = self->fileOps;
                }
                {
                    WorkspaceFileOpResult res;
                    res.kind      = req.kind;
                    res.requestId = req.requestId;
                    switch (req.kind)
                    {
                        case WorkspaceFileOpRequest::Kind::RevealExplorer:
                            res.outcome.sourcePath = req.sourcePath;
                            res.outcome.status     = reveal ? reveal->RevealInExplorer(req.sourcePath) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::RevealRedSalamander:
                            res.outcome.sourcePath = req.sourcePath;
                            res.outcome.status     = reveal ? reveal->RevealInRedSalamander(req.sourcePath, req.redSalamanderOverride) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::Rename:
                            res.outcome.status = ops ? ops->RenameItem(req.sourcePath, req.newLeafName, res.outcome) : E_FAIL;
                            break;
                        case WorkspaceFileOpRequest::Kind::Paste:
                            res.outcome.status =
                                ops ? ops->PasteItem(req.sourcePath, req.destinationPath, req.dropEffect, req.replaceExisting, req.keepBothCopy, res.outcome)
                                    : E_FAIL;
                            res.requiresCollisionPrompt = res.outcome.collision;
                            break;
                    }
                    std::lock_guard lock(self->mutex);
                    if (self->closing)
                        return;
                    self->completed.push_back(std::move(res));
                    if (self->window)
                        (void)PostMessageW(self->window, self->message, static_cast<WPARAM>(req.requestId), 0);
                    continue;
                }
            waitNext:
                if (WaitForSingleObject(self->wake.get(), INFINITE) != WAIT_OBJECT_0)
                    return;
            }
        }
        catch (const std::bad_alloc&)
        {
            DxUi::Failure::FailAllocation();
        }
        catch (const std::exception&)
        {
            DxUi::Failure::FailUnexpected();
        }
    }
};
#pragma warning(pop)

FileOpsService::~FileOpsService()
{
    Close();
}

HRESULT FileOpsService::Initialize(HWND window, UINT message, std::shared_ptr<IShellReveal> reveal, std::shared_ptr<IWorkspaceFileOps> fileOps) noexcept
try
{
    if ((window && message < WM_APP) || ! reveal || ! fileOps)
        return E_INVALIDARG;
    auto staged = std::make_shared<State>();
    staged->wake.reset(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (! staged->wake)
        return HRESULT_FROM_WIN32(GetLastError());
    staged->window   = window;
    staged->message  = message;
    staged->reveal   = std::move(reveal);
    staged->fileOps  = std::move(fileOps);
    auto ownership   = std::make_unique<ProcessWorkItem<State>>(staged);
    const HRESULT hr = ProcessWork::Acquire(ownership->lease);
    if (FAILED(hr))
        return hr;
    if (! TrySubmitThreadpoolCallback(State::Worker, ownership.get(), nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    (void)ownership.release();
    Close();
    _state = std::move(staged);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT FileOpsService::Submit(WorkspaceFileOpRequest request) noexcept
try
{
    if (! _state)
        return E_UNEXPECTED;
    std::lock_guard lock(_state->mutex);
    if (_state->closing)
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    if (_state->queue.size() >= 64u)
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    _state->busy = true;
    _state->queue.push_back(std::move(request));
    (void)SetEvent(_state->wake.get());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

std::optional<WorkspaceFileOpResult> FileOpsService::TakeResult()
{
    if (! _state)
        return std::nullopt;
    std::lock_guard lock(_state->mutex);
    if (_state->completed.empty())
        return std::nullopt;
    auto res = std::move(_state->completed.front());
    _state->completed.pop_front();
    return res;
}

bool FileOpsService::Busy() const noexcept
{
    if (! _state)
        return false;
    std::lock_guard lock(_state->mutex);
    return _state->busy || ! _state->queue.empty();
}

void FileOpsService::Stop() noexcept
{
    if (! _state)
        return;
    std::lock_guard lock(_state->mutex);
    _state->closing = true;
    _state->window  = nullptr;
    _state->queue.clear();
    (void)SetEvent(_state->wake.get());
}

void FileOpsService::Close() noexcept
{
    Stop();
    _state.reset();
}
} // namespace RedTilde
