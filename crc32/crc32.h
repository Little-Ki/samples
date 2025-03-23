#pragma once
#include <stdint.h>

uint32_t crc32(const uint8_t *data, size_t size);

uint32_t crc32(const char *data, size_t size);
