#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace RedTilde::Testing
{
// Independent uncompressed 24-bit BMP: two colored pixels in one padded row.
// Width one keeps the first pixel and padding; it decodes to four rather than
// eight BGRA bytes, making file-cache refresh independently observable.
inline std::vector<std::byte> ImageBitmapFixture(bool singlePixel = false)
{
    constexpr std::array<unsigned char, 62> bytes{0x42, 0x4d, 62, 0, 0, 0, 0, 0,  0, 0, 54, 0, 0, 0, 40,  0,   0, 0, 2, 0, 0,
                                                  0,    1,    0,  0, 0, 1, 0, 24, 0, 0, 0,  0, 0, 8, 0,   0,   0, 0, 0, 0, 0,
                                                  0,    0,    0,  0, 0, 0, 0, 0,  0, 0, 0,  0, 0, 0, 255, 255, 0, 0, 0, 0};
    std::vector<std::byte> result(bytes.size());
    std::memcpy(result.data(), bytes.data(), bytes.size());
    if (singlePixel)
        result[18] = std::byte{1};
    return result;
}
} // namespace RedTilde::Testing
