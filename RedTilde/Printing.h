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
#include <string_view>
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
    std::optional<unsigned> copies;
    std::optional<bool> collate;
};

enum class Orientation : uint8_t
{
    Portrait,
    Landscape
};

enum class Duplex : uint8_t
{
    OneSided,
    LongEdge,
    ShortEdge
};

struct PrinterSettings
{
    std::wstring form;
    std::optional<Orientation> orientation;
    std::optional<Duplex> duplex;
    std::optional<bool> collate;
    unsigned copies = 1u;
};

struct PrinterConfiguration
{
    Selection selection;
    PrinterSettings settings;
    std::vector<std::wstring> forms;
    unsigned maximumCopies = 1u;
};

struct Destinations
{
    std::vector<std::wstring> names;
    std::wstring selected;
};

[[nodiscard]] bool SamePrintName(std::wstring_view left, std::wstring_view right) noexcept;
[[nodiscard]] HRESULT ReadPaper(HDC dc, Paper &paper) noexcept;
[[nodiscard]] HRESULT ConfigurePrinter(const std::wstring &name, const std::optional<PrinterSettings> &requested, PrinterConfiguration &result);
[[nodiscard]] bool IsFilePort(std::wstring_view port) noexcept;
[[nodiscard]] std::optional<unsigned> ParsePageNumber(std::wstring_view text, unsigned pageCount) noexcept;
// An empty set selects all pages. A missing result is invalid input.
[[nodiscard]] std::optional<std::vector<PageRange>> ParsePageRanges(std::wstring_view text, unsigned pageCount);
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

enum class Stage
{
    Preview,
    ConfigurePrinter,
    ChooseFile,
    PrepareJob,
    Page,
    FinishJob,
    SaveFile
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
    Stage stage{};
    unsigned failedPage{};
    unsigned invalidRangePageCount{};
    bool savedFile{};
    std::wstring printer;
    std::optional<Destinations> destinations;
    // Present when the worker has attempted to configure a printer.
    std::optional<PrinterConfiguration> configuration;
    std::optional<PreviewPage> preview;
};

struct Job
{
    Document document;
    std::wstring initialPrinter;
    std::optional<PrinterSettings> initialSettings;
#if defined(REDTILDE_ENABLE_TESTS)
    std::optional<Destinations> destinationsForTest;
    std::function<HRESULT(const std::wstring &, const std::optional<PrinterSettings> &, PrinterConfiguration &)> printerForTest;
    // Receives the same closed Direct2D command lists used for driver submission.
    std::function<HRESULT(unsigned, ID2D1CommandList &)> pageForTest;
    std::function<HRESULT()> finishForTest;
    std::function<void(const Selection &)> submissionForTest;
#endif
};

struct Operation;
// One worker owns the prepared pages through preview, navigation and submission.
// The UI can detach and cancel without waiting in a window procedure.
[[nodiscard]] HRESULT Start(HWND owner, UINT message, Job job, std::shared_ptr<Operation> &operation);
[[nodiscard]] std::optional<Notice> TakeNotice(const std::shared_ptr<Operation> &operation);
void RequestPreview(Operation &operation, unsigned page) noexcept;
void RequestPrinter(Operation &operation, unsigned page, std::wstring printer, std::optional<PrinterSettings> settings = std::nullopt) noexcept;
void RequestPrint(Operation &operation, Selection selection) noexcept;
void ChooseFile(const std::shared_ptr<Operation> &operation, HRESULT result, std::filesystem::path destination) noexcept;
void Stop(std::shared_ptr<Operation> &operation) noexcept;
} // namespace RedTilde::Printing
