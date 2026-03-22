#pragma once
#include <stdint.h>

struct ZenithHeader {
    uint32_t version;
    uint32_t type;
    uint32_t payload_size;
    char filename[256];
};

#define PORT 8080