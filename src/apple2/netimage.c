#include "netimage.h"
#include "types.h"

// Platform specific hook to load image/palette data
uint8_t platform_load_image_block(const char* url, uint8_t block_type, uint32_t size, uint16_t block_index)
{
    // Stub for Apple II
    // Implement graphics loading here
    return 0;
}
