#include "stdint_shim.h"
#include "fujinet-network.h"
#include "graphics.h"
#include "netimage.h"
#include "utility.h"
#include "settings.h"
#include "types.h"
#include "vbxe.h"
#include "console.h"
#include "connection.h"

#include <atari.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>

extern byte buff[];
extern ImageData image;
extern struct __vbxe* VBXE;
extern Settings settings;

#define BLOCK_LEGACY 0xFF

uint8_t platform_load_image_block(const char* url, uint8_t block_type, uint32_t size, uint16_t block_index)
{
    static uint32_t image_offset = 0;

    // Reset offset if it's the first block (or logic to detect start)
    // Note: block_index comes from the loop in common/netimage.c.
    // If there are multiple types of blocks (palette, etc), index increments.
    // We should reset    static uint32_t image_offset = 0;

    // Debug: Print block index and current offset
    cprintf("Blk: %d Off: %ld\r\n", block_index, image_offset);
    
    // Check alignment
    if (((uint16_t)image.data & 0xFFF) != 0)
    {
        cprintf("ALIGN ERR: %p\r\n", image.data);
    }

    if (block_index == 0)
    {
        image_offset = 0;
        cprintf("Offset Reset\r\n");
    }
    
    // Safety check: Prevent buffer overflow
    if (image_offset >= 17000)
    {
        cprintf("Overflow! Resetting.\r\n");
        image_offset = 0;
    }

    if(block_type == BLOCK_LEGACY)
    {
        // Legacy loading logic (v2 < 4)
        #define BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE 4080
        #define BUFFER_ONE_BLOCK_THREE_SIZE 640

        if(!read_response_wait(url, (uint8_t*)image.data, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE)) return 1;

        if(!read_response_wait(url, (uint8_t*)image.data+0x1000, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE)) return 1;

        if(!read_response_wait(url, (uint8_t*)image.data+0x2000, BUFFER_ONE_BLOCK_THREE_SIZE)) return 1;
        
        return 0;
    }

    switch(block_type)
    {
        case IMAGE_BLOCK:  // Image data
            if ((settings.gfx_mode & 0x1F) >= GRAPHICS_20)
            {
                // VBXE Mode
                // Clear image
                clear_vbxe();

                // Enable the VBXE XDL.
                VBXE->VIDEO_CONTROL = 0x03;

                // Load the image data.
                {
                    uint16_t j;
                    for(j = 0; j < 19; ++j)
                    {
                        uint16_t byte_to_read = 4096;
                        if(j == 18)
                            byte_to_read = 3072;
                        VBXE->MEM_BANK_SEL = 128 + j;  // MOVE WINDOW STARTING $0000
                        if(!read_response_wait(url, XDL, byte_to_read))
                            return 1;
                    }
                }
            }
            else
            {
                // Standard Graphics Mode (8, 9, etc)
                // Read directly into image.data (framebuffer) with 4K boundary skipping
                // Each 4K page can hold 102 lines * 40 bytes = 4080 bytes.
                // We must skip the last 16 bytes of each 4K page.
                
                uint32_t bytes_remaining = size;
                uint32_t page_start;
                uint32_t page_limit;
                uint32_t space_in_page;
                uint16_t bytes_to_read;
                
                while (bytes_remaining > 0)
                {
                    // Check if we are at the end of a 4K page's usable area (4080 bytes)
                    if ((image_offset & 0xFFF) == 4080)
                    {
                        image_offset += 16; // Skip the 16-byte gap
                    }

                    // Calculate how many bytes we can write in the current page
                    // Usable space ends at offset 4080 within the page
                    page_start = image_offset & ~0xFFF;
                    page_limit = page_start + 4080;
                    space_in_page = page_limit - image_offset;
                    
                    // Determine read size for this chunk
                    bytes_to_read = (bytes_remaining < space_in_page) ? (uint16_t)bytes_remaining : (uint16_t)space_in_page;

                    if(!read_response_wait(url, (uint8_t*)image.data + image_offset, bytes_to_read))
                    {
                        show_error("Error reading image data\n");
                        return 1;
                    }

                    image_offset += bytes_to_read;
                    bytes_remaining -= bytes_to_read;
                }
            }
            break;
        case PALETTE_BLOCK:  // Palette
            VBXE->CSEL = 0x00;
            VBXE->PSEL = 0x01;
            {
                uint8_t* palette = (uint8_t*)malloc((uint16_t)size);
                if(NULL == palette)
                {
                    show_error("Error allocating palette buffer\n");
                    return 1;
                }
                if(!read_response_wait(url, palette, (uint16_t)size))
                {
                    show_error("Error reading palette\n");
                    free(palette);
                    return 1;
                }
                {
                    uint16_t j;
                    for(j = 0; j < (uint16_t)size; j+=3)
                    {
                        VBXE->CR = palette[j+0];
                        VBXE->CG = palette[j+1];
                        VBXE->CB = palette[j+2];
                    }
                }
                free(palette);
            }
            break;
        case XDL_BLOCK:
        default:
            // For now just consume or ignore.
            break;
    }

    return 0;
}
