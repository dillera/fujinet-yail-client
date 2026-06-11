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
#define FN_AIMODEL_KEY_ID 0x03

#define DEFAULT_GFX_MODE GRAPHICS_8
// Build-time override for local testing:
//   CFLAGS='-DYAIL_SERVER_URL=\"N:TCP://localhost:5556/\"' make TARGETS=atari all
#ifndef YAIL_SERVER_URL
#define YAIL_SERVER_URL "N:TCP://fujinet.org:5556/"
#endif
#define DEFAULT_URL YAIL_SERVER_URL
#define DEFAULT_AI_MODEL_NAME "dall-e-3"

// Globals
Settings settings = {
    DEFAULT_GFX_MODE,
    DEFAULT_URL,
    DEFAULT_AI_MODEL_NAME
};

// Externals
extern byte buff[];

// This requires that the default values are set in the arguments.
uint16_t read_or_create_appkey(uint8_t key_id, uint16_t len, char* data)
{
    uint16_t count = len;  // if the read fails, report the default length

    // Set the base key information
    fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);

    // Try to read the key
    if (fuji_read_appkey(key_id, &count, (uint8_t*)&buff[0]) > 0)
    {
        // Key was read so copy over the values
        memcpy(data, buff, count);
    }
    else
    {
        // Key doesn't exist. Write the default data
        count = len;
        fuji_write_appkey(key_id, len, (uint8_t*)data);
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

    count = strlen(DEFAULT_AI_MODEL_NAME) + 1;
    read_or_create_appkey((uint8_t)FN_AIMODEL_KEY_ID, count, (uint8_t *)&(settings.ai_model_name[0]));

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

            // Configure the key for writing
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);
            return fuji_write_appkey(FN_URL_KEY_ID, len, (uint8_t *)settings.url);
        }
        case SETTINGS_GFX:
        {
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);
            return fuji_write_appkey(FN_GFX_KEY_ID, 1, (uint8_t *)&settings.gfx_mode);
        }
        case SETTINGS_AI_MODEL:
        {
            uint16_t len = strlen(settings.ai_model_name) + 1;

            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);
            return fuji_write_appkey(FN_AIMODEL_KEY_ID, len, (uint8_t *)settings.ai_model_name);
        }
        default:
            return 0;
    }
}

void print_settings(uint8_t mode, char* url, char* ai_model)
{
    // Print the settings
    cputs("Settings:\n\r");
    cprintf("GFX: %02X %s\n\r", mode, graphics_mode_to_string(mode));
    cprintf("URL: %s\n\r", url);
    cprintf("MODEL: %s\n\r", ai_model);
}