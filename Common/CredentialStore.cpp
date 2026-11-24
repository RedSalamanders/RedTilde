#include "CredentialStore.h"
#include <DxUi/Failure.h>
#include <DxUi/RichDocument.h>
#include <cstring>
#include <wil/resource.h>
#include <wincred.h>

namespace RedTilde
{
namespace
{
using unique_cred = wil::unique_any<CREDENTIALW*, decltype(&CredFree), CredFree>;
} // namespace

void ScrubWideString(std::wstring& text) noexcept
{
    if (! text.empty())
        SecureZeroMemory(text.data(), text.size() * sizeof(wchar_t));
    text.clear();
}

HRESULT WindowsCredentialStore::Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept
try
{
    ScrubWideString(outCredential.secret);
    outCredential.username.clear();
    if (targetName.empty() || targetName.size() > 256u || ! DxUi::RichText::IsValidUtf16(targetName))
        return E_INVALIDARG;

    const std::wstring target(targetName);
    unique_cred cred;
    if (! CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, cred.put()))
        return HRESULT_FROM_WIN32(GetLastError());

    const auto* raw = cred.get();
    if (! raw)
        return E_FAIL;
    if (raw->UserName)
        outCredential.username = raw->UserName;
    if (raw->CredentialBlob && raw->CredentialBlobSize > 0u)
    {
        if (raw->CredentialBlobSize % sizeof(wchar_t) != 0u || raw->CredentialBlobSize > 8192u)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        std::wstring secret(reinterpret_cast<const wchar_t*>(raw->CredentialBlob), raw->CredentialBlobSize / sizeof(wchar_t));
        if (! DxUi::RichText::IsValidUtf16(secret))
        {
            ScrubWideString(secret);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
        outCredential.secret = std::move(secret);
    }
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT WindowsCredentialStore::Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept
try
{
    if (targetName.empty() || targetName.size() > 256u || secret.empty() || secret.size() > 4096u || ! DxUi::RichText::IsValidUtf16(targetName) ||
        ! DxUi::RichText::IsValidUtf16(username) || ! DxUi::RichText::IsValidUtf16(secret))
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
    if (targetName.empty() || targetName.size() > 256u)
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
    ScrubWideString(outCredential.secret);
    outCredential.username.clear();
    if (! _present || _target != targetName)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    outCredential = _credential;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MemoryCredentialStore::Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept
try
{
    if (targetName.empty() || secret.empty())
        return E_INVALIDARG;
    _target              = std::wstring(targetName);
    _credential.username = std::wstring(username);
    _credential.secret   = std::wstring(secret);
    _present             = true;
    return S_OK;
}
catch (const std::bad_alloc&)
{
    DxUi::Failure::FailAllocation();
}

HRESULT MemoryCredentialStore::Delete(std::wstring_view targetName) noexcept
{
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
