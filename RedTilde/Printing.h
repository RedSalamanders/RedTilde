#pragma once
#include <DxUi/MarkdownSyntax.h>
#include <DxUi/RichDocument.h>
#include <DxUi/RichImage.h>
#include <cstddef>
#include <d2d1_1.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <wil/resource.h>
#if defined(REDTILDE_ENABLE_TESTS)
#include <functional>
#endif

namespace RedTilde::Printing
{
struct PageRange
{
    unsigned first{}, last{};
};

// All geometry is in 1/96-inch units relative to the physical sheet, including
// the driver's unprintable offsets.
struct Paper
{
    D2D1_SIZE_F size{};
    D2D1_RECT_F printable{};
};

struct Selection
{
    std::wstring printer, port;
    std::vector<std::byte> deviceMode;
    Paper paper;
    std::vector<PageRange> ranges;
};

struct Dialog
{
    Dialog() noexcept = default;
    Dialog(const Dialog &) = delete;
    Dialog &operator=(const Dialog &) = delete;
    wil::unique_hglobal deviceMode, deviceNames;
    // S_FALSE dismisses or applies settings without starting a job.
    HRESULT Choose(HWND owner, Selection &selection);
};

[[nodiscard]] HRESULT ReadPaper(HDC dc, Paper &paper) noexcept;

struct Image
{
    DxUi::RichText::RichImageRequest request;
    std::shared_ptr<const DxUi::RichText::RichImage> pixels;
};

struct Document
{
    DxUi::RichText::SourceSnapshot source;
    std::filesystem::path path;
    std::wstring title;
    std::optional<DxUi::RichText::MarkdownExtensions> extensions;
    std::vector<Image> networkImages;
    bool allowRemoteImages{}, strikeCompletedTasks{};
};

enum class FileFormat
{
    DriverDefined,
    Pdf,
    OpenXps
};

struct Notice
{
    enum class Kind
    {
        ChooseFile,
        Finished
    };
    Kind kind{};
    FileFormat format{};
    HRESULT result{};
    unsigned invalidRangePageCount{};
    bool savedFile{};
};

struct Job
{
    Document document;
    Selection selection;
#if defined(REDTILDE_ENABLE_TESTS)
    // Receives the same closed Direct2D command lists used for driver submission.
    std::function<HRESULT(unsigned, ID2D1CommandList &)> pageForTest;
    std::function<HRESULT()> finishForTest;
#endif
};

struct Operation;
// The worker owns only the job values and this shared notification state. The
// UI can detach and cancel without waiting in a window procedure.
[[nodiscard]] HRESULT Start(HWND owner, UINT message, Job job, std::shared_ptr<Operation> &operation);
[[nodiscard]] std::optional<Notice> TakeNotice(const std::shared_ptr<Operation> &operation);
void ChooseFile(const std::shared_ptr<Operation> &operation, HRESULT result, std::filesystem::path destination) noexcept;
void Stop(std::shared_ptr<Operation> &operation) noexcept;
} // namespace RedTilde::Printing
