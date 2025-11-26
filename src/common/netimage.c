#include "stdint_shim.h"
#include "netimage.h"
#include "connection.h"
#include "app_errors.h"
#include "settings.h"
#include "console.h"
#include "utility.h"
#include "system.h"
#include "files.h"
#include "types.h"

#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// External globals
extern byte buff[];
extern ImageData image;
extern Settings settings;

void show_error_and_close_network(const char* message)
{
    show_error(message);
    disconnect_service(settings.url);
}

char check_keypress(ushort delay)
{
    // Wait for keypress
    char input;
    ushort i = 0;
    while(i++ < delay)   // roughly 5 seconds
        if(sys_key_pressed())
        {
            input = sys_get_key();
            if(CH_ENTER == input)   // pause
            {
                sys_get_key();            // any key to resume
                return(0x0);
            }
            else  // a key was pressed so let's assume it's a command and process it by quitting and returning the key
                show_console();
                return input;
        }

    return 0x0;
}

char stream_image(char* args[])
{
    ushort i = 0;
    char input;

    // Platform specific sound disable
    sys_sound_enable(false);

    #if DEBUG
    cprintf("reading from %s\n\r", settings.url);
    for(i = 0; args[i] != 0x0; i++)
        cprintf("%s ", args[i]);
    cputs("\n\r");
    #endif

    cprintf("Connecting to %s...\n\r", settings.url);

    if(!connect_service(settings.url))
    {
        show_error_and_close_network("Failed to connect\n\r");
        return 0x0;
    }

    // Send which graphics mode we are in
    memset(buff, 0, 256);
    // Note: GRAPHICS_CONSOLE_EN is platform specific constant, might need abstraction if value differs
    // For now assuming settings.gfx_mode is compatible or common enough
    sprintf((char*)buff, "gfx %d ", settings.gfx_mode & 0x7F); // Masking 0x80 (CONSOLE_EN often)
    if(!send_data(settings.url, buff, 6))
    {
        show_error_and_close_network("Unable to write graphics mode\n\r");
        return 0x0;
    }

    memset(buff, 0, 256);
    if(0 == strncmp(args[1], "http", 4))
    {
        // Build up the search string
        memcpy(buff, "showurl ", 8);
        for(i = 1; i < NUM_TOKENS; ++i)
        {
            if(0x0 == args[i])
                break;

            if(i > 0)
                strcat((char*)buff, " ");
            strcat((char*)buff, args[i]);
        }
    }
    else if((0 == strncmp(args[0], "search", 3) || (0 == strncmp(args[0], "gen", 3))) && (0 != args[1]))
    {
        // Build up the search string
        strncpy((char*)buff, args[0], 16);         
        for(i = 1; i < NUM_TOKENS; ++i)
        {
            if(0x0 == args[i])
                break;

            if(i > 1)
                strcat((char*)buff, " ");
            else
                strcat((char*)buff, " \"");
            strcat((char*)buff, args[i]);
        }
        if(i > 0)
            strcat((char*)buff, "\"");
    }
    else if(0 == strncmp(args[0], "video", 3))
        memcpy(buff, "video", 5);
    else  // no search terms provided for the stream command so just stream files from the server (path must be set on server startup)
        memcpy(buff, "files", 5);

    i = strlen((char*)buff);

    if(!send_data(settings.url, buff, i))
    {
        show_error_and_close_network("Unable to write request\n\r");
        return 0x0;
    }

    sys_disable_screensaver();

    hide_console();

    while(true)
    {
        // Read the header
        if(!read_response_wait(settings.url, (unsigned char*)&image.header, sizeof(image.header)))
        {
            show_error_and_close_network("Error reading header\n\r");
            break;
        }

        // Read the block information, unused for now
        if(image.header.v2 < 4)
        {
            if(!read_response_wait(settings.url, buff, sizeof(BlockHeaderV13)))
            {
                show_error_and_close_network("Error reading block header\n\r");
                break;
            }
        }

        // Read the image data
        // Call platform specific hook to handle the rest of the image/block loading
        // We pass the URL so the platform code can continue reading from the socket
        // logic for v2 >= 4 blocks is complex (loops, types), so it's better if platform handles it 
        // OR we interpret blocks here and call hooks for DATA.
        // But the block structure determines HOW MUCH to read.
        
        // Let's try to handle the block loop here if possible?
        // The Atari code handles v2 < 4 differently than v2 >= 4.
        // v2 >= 4 has block headers.
        
        if(image.header.v2 >= 4)
        {
             uint8_t num_blocks;
             uint8_t b;

             if(!read_response_wait(settings.url, &num_blocks, sizeof(num_blocks))) {
                 show_error_and_close_network("Error reading num blocks\n\r");
                 break;
             }
             
             // Platform hook to prepare for blocks (e.g. clear screen)
             // Maybe we need a prepare_image_render() hook?
             // For now, we can't easily abstract the loop without reading block headers here.
             // So let's read block headers here.
             
             for(b = 0; b < num_blocks; ++b)
             {
                 BlockHeaderV14 block_header;
                 if(!read_response_wait(settings.url, (uint8_t*)&block_header, sizeof(block_header))) {
                     show_error_and_close_network("Error reading block header\n\r");
                     goto quit;
                 }
                 
                 if(block_header.block_type == ERROR_BLOCK)
                 {
                     if(!read_response_wait(settings.url, (uint8_t*)buff, (uint16_t)block_header.size)) {
                         show_error_and_close_network("Error reading error message\n\r");
                         goto quit;
                     }
                     buff[block_header.size] = 0;
                     cprintf("Error: %s\n", buff);
                 }
                 else
                 {
                     // Dispatch to platform to load the CONTENT of the block
                     uint8_t res = platform_load_image_block((const char*)settings.url, (uint8_t)block_header.block_type, (uint32_t)block_header.size, (uint16_t)b);
                     if(res != 0) // Error
                     {
                         // Error handled in platform or just return
                         goto quit;
                     }
                 }
             }
        }
        else
        {
            // Legacy format
            // Pass a special block type or just call a legacy hook?
            // Reusing platform_load_image_block with a LEGACY type might work.
            // But the size is implied by image.header for legacy? 
            // Actually legacy reads fixed chunks.
            // Let's call platform hook with LEGACY type.
            #define BLOCK_LEGACY 0xFF
            uint8_t res = platform_load_image_block((const char*)settings.url, (uint8_t)BLOCK_LEGACY, (uint32_t)0, (uint16_t)0);
            if(res != 0) goto quit;
        }

        input = check_keypress(30000);
        if(input)
            goto quit;

        if(!send_data(settings.url, (uint8_t*)"next", 4))
        {
            show_error("Unable to write request\n\r");
            break;
        }

        sys_disable_screensaver(); // Ping screensaver
    }

quit:
    send_data(settings.url, (uint8_t*)"quit", 4);

    disconnect_service(settings.url);

    sys_sound_enable(true);

    sys_enable_screensaver();

    return input;
}
