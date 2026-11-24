#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace RedTilde
{
enum class DropEffectKind : std::uint8_t
{
    None = 0,
    Copy = 1,
    Move = 2
};

struct FileClipboardPayload
{
    std::vector<std::filesystem::path> paths;
    DropEffectKind effect{DropEffectKind::Copy};
};

struct ShellRevealTarget
{
    enum class Tool : std::uint8_t
    {
        Explorer      = 0,
        RedSalamander = 1
    };
    Tool tool{Tool::Explorer};
    std::filesystem::path targetPath;
    std::optional<std::filesystem::path> redSalamanderOverride;
};

class IShellReveal
{
public:
    virtual ~IShellReveal() = default;
    [[nodiscard]] virtual std::optional<std::filesystem::path> FindRedSalamander(const std::optional<std::filesystem::path>& overridePath) const noexcept = 0;
    [[nodiscard]] virtual HRESULT RevealInExplorer(const std::filesystem::path& targetPath) noexcept                                                      = 0;
    [[nodiscard]] virtual HRESULT RevealInRedSalamander(const std::filesystem::path& targetPath,
                                                        const std::optional<std::filesystem::path>& overridePath) noexcept                                = 0;
};

class WindowsShellReveal final : public IShellReveal
{
public:
    [[nodiscard]] std::optional<std::filesystem::path> FindRedSalamander(const std::optional<std::filesystem::path>& overridePath) const noexcept override;
    [[nodiscard]] HRESULT RevealInExplorer(const std::filesystem::path& targetPath) noexcept override;
    [[nodiscard]] HRESULT RevealInRedSalamander(const std::filesystem::path& targetPath,
                                                const std::optional<std::filesystem::path>& overridePath) noexcept override;
};

enum class FileNameValidationError : std::uint8_t
{
    None = 0,
    Empty,
    TooLong,
    InvalidCharacters,
    PathSeparator,
    ReservedName,
    TrailingDotOrSpace
};

struct FileOperationOutcome
{
    HRESULT status{S_OK};
    std::filesystem::path sourcePath;
    std::filesystem::path destinationPath;
    bool collision{};
    bool reparseRejected{};
};

class IWorkspaceFileOps
{
public:
    virtual ~IWorkspaceFileOps()                                                                               = default;
    [[nodiscard]] virtual FileNameValidationError ValidateFileName(std::wstring_view candidate) const noexcept = 0;
    [[nodiscard]] virtual HRESULT RenameItem(const std::filesystem::path& sourcePath,
                                             std::wstring_view newLeafName,
                                             FileOperationOutcome& outOutcome) noexcept                        = 0;
    [[nodiscard]] virtual HRESULT PasteItem(const std::filesystem::path& sourcePath,
                                            const std::filesystem::path& destinationDirectory,
                                            DropEffectKind effect,
                                            bool replaceExisting,
                                            bool keepBothCopy,
                                            FileOperationOutcome& outOutcome) noexcept                         = 0;
};

class WindowsWorkspaceFileOps final : public IWorkspaceFileOps
{
public:
    [[nodiscard]] FileNameValidationError ValidateFileName(std::wstring_view candidate) const noexcept override;
    [[nodiscard]] HRESULT RenameItem(const std::filesystem::path& sourcePath,
                                     std::wstring_view newLeafName,
                                     FileOperationOutcome& outOutcome) noexcept override;
    [[nodiscard]] HRESULT PasteItem(const std::filesystem::path& sourcePath,
                                    const std::filesystem::path& destinationDirectory,
                                    DropEffectKind effect,
                                    bool replaceExisting,
                                    bool keepBothCopy,
                                    FileOperationOutcome& outOutcome) noexcept override;
};

[[nodiscard]] HRESULT WriteDropFilesToClipboard(HWND owner, const FileClipboardPayload& payload) noexcept;
[[nodiscard]] HRESULT ReadDropFilesFromClipboard(HWND owner, FileClipboardPayload& outPayload) noexcept;
[[nodiscard]] HRESULT WriteUnicodeTextToClipboard(HWND owner, std::wstring_view text) noexcept;
[[nodiscard]] std::wstring QuoteWindowsCliArgument(std::wstring_view argument);

struct WorkspaceFileOpRequest
{
    enum class Kind : std::uint8_t
    {
        RevealExplorer      = 0,
        RevealRedSalamander = 1,
        Rename              = 2,
        Paste               = 3
    };
    Kind kind{Kind::RevealExplorer};
    std::uint64_t requestId{};
    std::filesystem::path sourcePath;
    std::filesystem::path destinationPath;
    std::wstring newLeafName;
    std::optional<std::filesystem::path> redSalamanderOverride;
    DropEffectKind dropEffect{DropEffectKind::Copy};
    bool replaceExisting{};
    bool keepBothCopy{};
};

struct WorkspaceFileOpResult
{
    WorkspaceFileOpRequest::Kind kind{WorkspaceFileOpRequest::Kind::RevealExplorer};
    std::uint64_t requestId{};
    FileOperationOutcome outcome;
    bool requiresCollisionPrompt{};
};

// Event-driven ProcessWork-leased background worker for reveal, rename, copy, and move.
class FileOpsService final
{
public:
    FileOpsService() = default;
    ~FileOpsService();
    FileOpsService(const FileOpsService&)            = delete;
    FileOpsService& operator=(const FileOpsService&) = delete;

    [[nodiscard]] HRESULT Initialize(HWND window, UINT message, std::shared_ptr<IShellReveal> reveal, std::shared_ptr<IWorkspaceFileOps> fileOps) noexcept;
    [[nodiscard]] HRESULT Submit(WorkspaceFileOpRequest request) noexcept;
    [[nodiscard]] std::optional<WorkspaceFileOpResult> TakeResult();
    [[nodiscard]] bool Busy() const noexcept;
    void Stop() noexcept;
    void Close() noexcept;

private:
    struct State;
    std::shared_ptr<State> _state;
};
} // namespace RedTilde
