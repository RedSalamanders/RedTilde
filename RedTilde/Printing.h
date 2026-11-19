#pragma once
#include <DxUi/MarkdownSyntax.h>
#include <DxUi/RichDocument.h>
#include <DxUi/RichImage.h>
#include <cstddef>
#include <cstdint>
#include <d2d1_1.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
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

    [[nodiscard]] bool operator==(const Paper &other) const noexcept;
};

struct Selection
{
    std::wstring printer, port;
    std::vector<std::byte> deviceMode;
    Paper paper;
    std::vector<PageRange> ranges;
};

struct DialogOptions
{
    unsigned pageCount{};
    std::span<const PageRange> ranges;
    bool rangeNeedsCorrection{};
    bool currentPage{}, printToFile{};
};

struct DialogResult
{
    Selection selection;
    bool confirmed{}, currentPage{}, printToFile{};
};

struct Dialog
{
    Dialog() noexcept = default;
    Dialog(const Dialog &) = delete;
    Dialog &operator=(const Dialog &) = delete;
    wil::unique_hglobal deviceMode, deviceNames;
    // S_FALSE dismisses the dialog. Apply returns settings with confirmed=false.
    HRESULT Choose(HWND owner, const DialogOptions &options, DialogResult &result);
};

[[nodiscard]] HRESULT ReadPaper(HDC dc, Paper &paper) noexcept;
[[nodiscard]] bool PageRangesFit(std::span<const PageRange> ranges, unsigned pageCount) noexcept;

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

struct PreviewPage
{
    Paper paper;
    unsigned number{}, count{};
    UINT width{}, height{};
    std::vector<std::uint8_t> bgra;
    bool fallbackPaper{};
};

struct Notice
{
    enum class Kind
    {
        Preview,
        ChooseFile,
        Finished
    };
    Kind kind{};
    FileFormat format{};
    HRESULT result{};
    unsigned invalidRangePageCount{};
    bool savedFile{};
    std::optional<PreviewPage> preview;
};

struct Job
{
    Document document;
    std::optional<Paper> initialPaper;
#if defined(REDTILDE_ENABLE_TESTS)
    // Receives the same closed Direct2D command lists used for driver submission.
    std::function<HRESULT(unsigned, ID2D1CommandList &)> pageForTest;
    std::function<HRESULT()> finishForTest;
#endif
};

struct Operation;
// One worker owns the prepared pages through preview, navigation and submission.
// The UI can detach and cancel without waiting in a window procedure.
[[nodiscard]] HRESULT Start(HWND owner, UINT message, Job job, std::shared_ptr<Operation> &operation);
[[nodiscard]] std::optional<Notice> TakeNotice(const std::shared_ptr<Operation> &operation);
void RequestPreview(Operation &operation, unsigned page, std::optional<Paper> paper = std::nullopt) noexcept;
void RequestPrint(Operation &operation, Selection selection) noexcept;
void ChooseFile(const std::shared_ptr<Operation> &operation, HRESULT result, std::filesystem::path destination) noexcept;
void Stop(std::shared_ptr<Operation> &operation) noexcept;
} // namespace RedTilde::Printing
