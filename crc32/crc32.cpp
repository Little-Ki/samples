#include "crc32.h"

#include <algorithm>
#include <array>

std::array<uint32_t, 256> generate_crc_table() noexcept {
    constexpr uint32_t poly = 0xEDB88320;

    auto table = std::array<uint32_t, 256>{};
    std::generate(table.begin(), table.end(), [n = 0]() mutable {
        uint32_t t = n++;
        for (int j = 8; j > 0; j--)
            t = (t >> 1) ^ ((t & 1) ? poly : 0);
        return t;
    });

    return table;
}

uint32_t crc32(const uint8_t *data, size_t size) {
    static const auto table = generate_crc_table();

    uint32_t crc = 0xFFFFFFFF;
    while (size--) {
        crc = ((crc >> 8) & 0x00FFFFFF) ^ table[(crc ^ (*data)) & 0xFF];
        data++;
    }
    return (crc ^ 0xFFFFFFFF);
}

uint32_t crc32(const char *data, size_t size) {
    return crc32(reinterpret_cast<const uint8_t *>(data), size);
}
