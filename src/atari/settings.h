#ifndef SETTINGS_H
#define SETTINGS_H

#include "types.h"
#include "fujinet-fuji.h"

//
#define SERVER_URL_SIZE MAX_APPKEY_LEN

#define SETTINGS_NONE 0
#define SETTINGS_URL  1
#define SETTINGS_GFX  2

// A structure for holding the application runtime settings
typedef struct {
    byte gfx_mode;
    char url[SERVER_URL_SIZE];
} Settings;

//unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key);

uint8_t get_settings(void);

uint8_t put_settings(byte select);

void print_settings(uint8_t mode, char* url);

#endif