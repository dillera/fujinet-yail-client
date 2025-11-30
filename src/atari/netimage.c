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
    static uint16_t image_offset = 0;

    // Reset offset if it's the first block (or logic to detect start)
    // Note: block_index comes from the loop in common/netimage.c.
    // If there are multiple types of blocks (palette, etc), index increments.
    // We should reset    static uint32_t image_offset = 0;

    // Debug: Print block index, type, size, offset
    // cprintf("L B%d T%d S%ld\r\n", block_index, block_type, size);
    
    // Check alignment
    if (((uint16_t)image.data & 0xFFF) != 0)
    {
        // cprintf("ALIGN ERR: %p\r\n", image.data);
    }

    if (block_index == 0)
    {
        image_offset = 0;
        // cprintf("Offset Reset\r\n");
    }
    
    // Safety check to prevent buffer overflow
    {
        // VBXE modes need more space (320x240 = 76800 bytes)
        uint32_t max_offset = 17000;
        if ((settings.gfx_mode & 0x1F) >= GRAPHICS_20)
            max_offset = 80000;

        if (image_offset >= max_offset)
        {
            // cprintf("Offset Reset R M%x O%ld\r\n", settings.gfx_mode, image_offset);
            image_offset = 0;
        }
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
                if (!VBXE)
                {
                    cprintf("VBXE NOT DETECTED!\r\n");
                    return 1;
                }

                // Clear image only at the start of IMAGE data
                if (image_offset == 0)
                {
                    clear_vbxe();
                    // Enable the VBXE XDL.
                    VBXE->VIDEO_CONTROL = 0x03;
                }

                // Load the image data chunk
                {
                    uint32_t bytes_remaining = size;
                    uint16_t bytes_to_read;
                    uint8_t bank;
                    uint16_t bank_offset;
                    uint16_t space_in_bank;

                    while (bytes_remaining > 0)
                    {
                        // Calculate bank and offset
                        bank = (uint8_t)((image_offset >> 12) & 0xFF);
                        bank_offset = image_offset & 0xFFF;

                        // Select bank
                        VBXE->MEM_BANK_SEL = 128 + bank;  // MOVE WINDOW STARTING $0000 (Window at 0x8000 via XDL pointer)

                        // Calculate how many bytes we can write in this bank
                        space_in_bank = 4096 - bank_offset;
                        
                        // Determine read size for this chunk
                        bytes_to_read = (bytes_remaining < space_in_bank) ? (uint16_t)bytes_remaining : space_in_bank;

                        // Read into the XDL window (0x8000) + offset
                        if(!read_response_wait(url, XDL + bank_offset, bytes_to_read))
                        {
                            show_error("Error reading VBXE data\n");
                            return 1;
                        }

                        image_offset += bytes_to_read;
                        bytes_remaining -= bytes_to_read;
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
            {
                uint16_t i;
                uint8_t rgb[3];

                // Select Palette 1 (used by XDL)
                VBXE->PSEL = 1;

                // Read 256 RGB triplets (768 bytes)
                for(i = 0; i < 256; ++i)
                {
                    // Read RGB triplet
                    if(!read_response_wait(url, rgb, 3)) return 1;

                    // Set Color Index
                    VBXE->CSEL = (uint8_t)i;
                    
                    // Write RGB
                    VBXE->CR = rgb[0];
                    VBXE->CG = rgb[1];
                    VBXE->CB = rgb[2];
                }
            }
            break;
        case XDL_BLOCK:
        default:
            // Consume the data to keep stream synchronized
            {
                uint32_t bytes_remaining = size;
                uint16_t bytes_to_read;
                // Use global buff to avoid "Too many local variables" error
                // uint8_t trash[256];

                while (bytes_remaining > 0)
                {
                    bytes_to_read = (bytes_remaining < 256) ? (uint16_t)bytes_remaining : 256;
                    if(!read_response_wait(url, (uint8_t*)buff, bytes_to_read))
                    {
                        show_error("Error skipping block data\n");
                        return 1;
                    }
                    bytes_remaining -= bytes_to_read;
                }
            }
            break;
    }

    return 0;
}
