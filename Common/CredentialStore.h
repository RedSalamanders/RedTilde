#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <windows.h>

namespace RedTilde
{
inline constexpr std::wstring_view GitHubCredentialTarget = L"RedTilde/GitHub/api.github.com";

struct StoredCredential
{
    std::wstring username;
    std::wstring secret;
};

class ICredentialStore
{
public:
    virtual ~ICredentialStore()                                                                                                      = default;
    [[nodiscard]] virtual HRESULT Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept                       = 0;
    [[nodiscard]] virtual HRESULT Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept = 0;
    [[nodiscard]] virtual HRESULT Delete(std::wstring_view targetName) noexcept                                                      = 0;
};

class WindowsCredentialStore final : public ICredentialStore
{
public:
    [[nodiscard]] HRESULT Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept override;
    [[nodiscard]] HRESULT Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept override;
    [[nodiscard]] HRESULT Delete(std::wstring_view targetName) noexcept override;
};

class MemoryCredentialStore final : public ICredentialStore
{
public:
    [[nodiscard]] HRESULT Read(std::wstring_view targetName, StoredCredential& outCredential) noexcept override;
    [[nodiscard]] HRESULT Write(std::wstring_view targetName, std::wstring_view username, std::wstring_view secret) noexcept override;
    [[nodiscard]] HRESULT Delete(std::wstring_view targetName) noexcept override;

private:
    std::wstring _target;
    StoredCredential _credential;
    bool _present{};
};

void ScrubWideString(std::wstring& text) noexcept;
} // namespace RedTilde
