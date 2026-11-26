#include "CredentialStore.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <wil/resource.h>
#include <wincred.h>

namespace RedTilde
{
static_assert(MaximumCredentialSecretChars * sizeof(wchar_t) == CRED_MAX_CREDENTIAL_BLOB_SIZE);

namespace
{
using unique_cred = wil::unique_any<CREDENTIALW*, decltype(&CredFree), CredFree>;

[[nodiscard]] bool HasControlOrNul(std::wstring_view text) noexcept
{
    return std::ranges::any_of(text, [](wchar_t ch) noexcept { return ch == L'\0' || static_cast<unsigned>(ch) < 0x20u || ch == 0x7Fu; });
}

[[nodiscard]] bool ValidTargetName(std::wstring_view targetName) noexcept
{
    return ! targetName.empty() && targetName.size() <= 256u && DxUi::RichText::IsValidUtf16(targetName) && ! HasControlOrNul(targetName);
}

[[nodiscard]] bool ValidUsername(std::wstring_view username) noexcept
{
    return username.size() <= 256u && DxUi::RichText::IsValidUtf16(username) && ! HasControlOrNul(username);
}

[[nodiscard]] bool ValidSecret(std::wstring_view secret) noexcept
{
    return ! secret.empty() && secret.size() <= MaximumCredentialSecretChars && secret.find(L'\0') == std::wstring_view::npos &&
           DxUi::RichText::IsValidUtf16(secret);
}
} // namespace

ScrubbedWideSecret::~ScrubbedWideSecret() noexcept
{
    Clear();
}

ScrubbedWideSecret::ScrubbedWideSecret(ScrubbedWideSecret&& other) noexcept : _data(std::move(other._data)), _size(std::exchange(other._size, 0u))
{
}

ScrubbedWideSecret& ScrubbedWideSecret::operator=(ScrubbedWideSecret&& other) noexcept
{
    if (this != &other)
    {
        Clear();
        _data = std::move(other._data);
        _size = std::exchange(other._size, 0u);
    }
    return *this;
}

void ScrubbedWideSecret::Assign(std::wstring_view value)
{
    std::unique_ptr<wchar_t[]> next;
    if (! value.empty())
    {
        next = std::make_unique_for_overwrite<wchar_t[]>(value.size());
        std::memcpy(next.get(), value.data(), value.size() * sizeof(wchar_t));
    }
    Clear();
    _data = std::move(next);
    _size = value.size();
}

void ScrubbedWideSecret::Clear() noexcept
{
    if (_data && _size > 0u)
        SecureZeroMemory(_data.get(), _size * sizeof(wchar_t));
    _data.reset();
    _size = 0u;
}

bool ScrubbedWideSecret::Empty() const noexcept
{
    return _size == 0u;
}

std::wstring_view ScrubbedWideSecret::View() const noexcept
{
    return _data && _size > 0u ? std::wstring_view(_data.get(), _size) : std::wstring_view{};
}

void ScrubWideString(std::wstring& text) noexcept
{
    if (! text.empty())
        SecureZeroMemory(text.data(), text.size() * sizeof(wchar_t));
    text.clear();
}

void ScrubString(std::string& text) noexcept
{
    if (! text.empty())
        SecureZeroMemory(text.data(), text.size());
    text.clear();
}

HRESULT WindowsCredentialStore::Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept
try
{
    if (! ValidTargetName(targetName))
        return E_INVALIDARG;

    const std::wstring target(targetName);
    unique_cred cred;
    if (! CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, cred.put()))
        return HRESULT_FROM_WIN32(GetLastError());

    auto* const raw      = cred.get();
    const auto scrubCred = wil::scope_exit([raw]() noexcept
    {
        if (raw && raw->CredentialBlob && raw->CredentialBlobSize > 0u)
            SecureZeroMemory(raw->CredentialBlob, raw->CredentialBlobSize);
    });
    if (! raw)
        return E_FAIL;

    StoredCredential staged;
    const auto scrubStaged = wil::scope_exit([&]() noexcept { ScrubWideString(staged.secret); });
    if (raw->UserName)
    {
        const std::wstring_view userView(raw->UserName);
        if (! ValidUsername(userView))
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        staged.username.assign(userView);
    }
    if (! raw->CredentialBlob || raw->CredentialBlobSize == 0u || raw->CredentialBlobSize % sizeof(wchar_t) != 0u ||
        raw->CredentialBlobSize > CRED_MAX_CREDENTIAL_BLOB_SIZE)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    const std::size_t wcharCount = static_cast<std::size_t>(raw->CredentialBlobSize) / sizeof(wchar_t);
    staged.secret.assign(wcharCount, L'\0');
    std::memcpy(staged.secret.data(), raw->CredentialBlob, raw->CredentialBlobSize);
    if (! ValidSecret(staged.secret))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    ScrubWideString(outCredential.secret);
    outCredential.username = std::move(staged.username);
    outCredential.secret.assign(staged.secret);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsCredentialStore::Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept
try
{
    if (! ValidTargetName(targetName) || ! ValidUsername(username) || ! ValidSecret(secret))
        return E_INVALIDARG;

    std::wstring target(targetName);
    std::wstring user(username);
    std::wstring secretCopy(secret);
    const auto scrub = wil::scope_exit([&]() noexcept { ScrubWideString(secretCopy); });

    CREDENTIALW cred{};
    cred.Type               = CRED_TYPE_GENERIC;
    cred.TargetName         = target.data();
    cred.UserName           = user.empty() ? nullptr : user.data();
    cred.CredentialBlobSize = static_cast<DWORD>(secretCopy.size() * sizeof(wchar_t));
    cred.CredentialBlob     = reinterpret_cast<LPBYTE>(secretCopy.data());
    cred.Persist            = CRED_PERSIST_LOCAL_MACHINE;
    if (! CredWriteW(&cred, 0))
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsCredentialStore::Delete(std::wstring_view targetName) noexcept
try
{
    if (! ValidTargetName(targetName))
        return E_INVALIDARG;
    const std::wstring target(targetName);
    if (! CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0))
    {
        const DWORD err = GetLastError();
        if (err == ERROR_NOT_FOUND)
            return S_OK;
        return HRESULT_FROM_WIN32(err);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MemoryCredentialStore::Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept
try
{
    if (! ValidTargetName(targetName))
        return E_INVALIDARG;
    if (! _present || _target != targetName)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    if (! ValidUsername(_credential.username) || ! ValidSecret(_credential.secret))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    std::wstring stagedUser(_credential.username);
    std::wstring stagedSecret(_credential.secret);
    const auto scrubStaged = wil::scope_exit([&]() noexcept { ScrubWideString(stagedSecret); });
    ScrubWideString(outCredential.secret);
    outCredential.username = std::move(stagedUser);
    outCredential.secret.assign(stagedSecret);
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MemoryCredentialStore::Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept
try
{
    if (! ValidTargetName(targetName) || ! ValidUsername(username) || ! ValidSecret(secret))
        return E_INVALIDARG;
    std::wstring stagedTarget(targetName);
    std::wstring stagedUser(username);
    std::wstring stagedSecret(secret);
    const auto scrubStaged = wil::scope_exit([&]() noexcept { ScrubWideString(stagedSecret); });
    ScrubWideString(_credential.secret);
    _target              = std::move(stagedTarget);
    _credential.username = std::move(stagedUser);
    _credential.secret.assign(stagedSecret);
    _present = true;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MemoryCredentialStore::Delete(std::wstring_view targetName) noexcept
{
    if (! ValidTargetName(targetName))
        return E_INVALIDARG;
    if (_present && _target == targetName)
    {
        ScrubWideString(_credential.secret);
        _credential.username.clear();
        _target.clear();
        _present = false;
    }
    return S_OK;
}
} // namespace RedTilde
