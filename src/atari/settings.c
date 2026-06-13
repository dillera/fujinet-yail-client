#include "settings.h"
#include "graphics.h"
#include "utility.h"
#include "fujinet-fuji.h"

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <atari.h>

#define FN_CREATOR_ID 0xD00D
#define FN_APP_ID 0x01
#define FN_URL_KEY_ID 0x01
#define FN_GFX_KEY_ID 0x02

#define DEFAULT_GFX_MODE GRAPHICS_8
// Build-time override for local testing:
//   CFLAGS='-DYAIL_SERVER_URL=\"N:TCP://localhost:5556/\"' make TARGETS=atari all
#ifndef YAIL_SERVER_URL
#define YAIL_SERVER_URL "N:TCP://fujinet.org:5556/"
#endif
#define DEFAULT_URL YAIL_SERVER_URL

// Globals
Settings settings = {
    DEFAULT_GFX_MODE,
    DEFAULT_URL
};

// Externals
extern byte buff[];

// Read an appkey, creating it with the default data passed in `data` if it
// does not exist yet. `data` must already hold the default value; on a
// successful read it is overwritten with the stored value. Returns the number
// of bytes now in `data`.
//
// On first boot none of the keys exist, so the read fails (FujiNet logs
// errno=2 "file not found") and we write the default to create the key. The
// third arg to fuji_set_appkey_details is an enum AppKeySize (DEFAULT = 64
// byte keys), NOT a byte length.
uint16_t read_or_create_appkey(uint8_t key_id, uint16_t len, char* data)
{
    uint16_t count = len;  // if the read fails, report the default length

    // Set the base key information
    fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, DEFAULT);

    // Try to read the key
    if (fuji_read_appkey(key_id, &count, (uint8_t*)&buff[0]))
    {
        // Key was read so copy over the values
        memcpy(data, buff, count);
    }
    else
    {
        // Key doesn't exist yet (expected on first boot). Create it by
        // writing the default data that is already in `data`.
        count = len;
        if (!fuji_write_appkey(key_id, len, (uint8_t*)data))
        {
            // Persisting failed; the in-RAM default is still used so the
            // app runs, but the key will be re-created on the next boot.
            show_error("Could not save setting to FujiNet\n\r");
        }
    }

    return count;
}

uint8_t get_settings()
{
    uint16_t count; // Max key size

    count = 1;
    read_or_create_appkey((uint8_t)FN_GFX_KEY_ID, count, (uint8_t *)&settings.gfx_mode);

    count = strlen(DEFAULT_URL) + 1;
    read_or_create_appkey((uint8_t)FN_URL_KEY_ID, count, (uint8_t *)&(settings.url[0]));

    // Add more settings below...

    // Apply the graphics mode setting.  Really should be done outside of here.
    setGraphicsMode(settings.gfx_mode);

    return 0;
}

uint8_t put_settings(byte select)
{
    switch(select)
    {
        case SETTINGS_NONE:
            return 0;
        case SETTINGS_URL:
        {
            uint16_t len = strlen(settings.url) + 1;

            // Configure the key for writing. The keysize arg is an enum
            // AppKeySize (DEFAULT = 64 byte keys), and must match the value
            // used by read_or_create_appkey() so reads and writes line up.
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, DEFAULT);
            return fuji_write_appkey(FN_URL_KEY_ID, len, (uint8_t *)settings.url);
        }
        case SETTINGS_GFX:
        {
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, DEFAULT);
            return fuji_write_appkey(FN_GFX_KEY_ID, 1, (uint8_t *)&settings.gfx_mode);
        }
        default:
            return 0;
    }
}

void print_settings(uint8_t mode, char* url)
{
    // Print the settings
    cputs("Settings:\n\r");
    cprintf("GFX: %02X %s\n\r", mode, graphics_mode_to_string(mode));
    cprintf("URL: %s\n\r", url);
}