#include "settings.h"
#include "types.h"
#include <string.h>

extern Settings settings;

uint8_t get_settings(void)
{
    // Set defaults
    #ifdef YAIL_URL
    strncpy(settings.url, YAIL_URL, SERVER_URL_SIZE);
    #else
    strncpy(settings.url, "N:TCP://fujinet.online:5556/", SERVER_URL_SIZE);
    #endif
    // settings.gfx_mode = GRAPHICS_8; // Not applicable for Apple II yet
    return 0;
}

uint8_t put_settings(byte select)
{
    return 0;
}
