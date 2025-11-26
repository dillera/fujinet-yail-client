// Copyright (C) 2021 Brad Colbert

#ifndef NETIMAGE_H
#define NETIMAGE_H

#include "types.h"
#include "stdint_shim.h"

// Defines
#define CONSOLE_LINES 5
#define CONSOLE_HIDDEN 0
#define CONSOLE_SHOWN 1
#define MAX_LINE_LEN 40

// Block types
#define IMAGE_BLOCK 0
#define PALETTE_BLOCK 1
#define ERROR_BLOCK 2
#define XDL_BLOCK 3

typedef struct
{
    uint16_t size;
    uint8_t block_type;
} BlockHeaderV13;

typedef struct
{
    uint32_t size;
    uint8_t block_type;
} BlockHeaderV14;

signed char enable_network(const char* url);
signed char disable_network(const char* url);
signed char check_network(const char* url);
signed char write_network(const char* url, const char* buf, unsigned short len);
signed char read_network(const char* url, unsigned char* buf, unsigned short len);
char stream_image(char* args[]);
void show_image(char* args[]);

// Platform specific hook to load image/palette data
// block_index is used to track state (e.g. offset) across multiple blocks
uint8_t platform_load_image_block(const char* url, uint8_t block_type, uint32_t size, uint16_t block_index);

#endif // NETIMAGE_H