#include "system.h"
#include "settings.h"
#include "graphics.h"
#include "utility.h"
#include "version.h"
#include "vbxe.h"
#include <atari.h>
#include <conio.h>
#include <stdbool.h>

extern char version[];
extern ushort ORIG_VBII_SAVE;

void sys_init(void)
{
    // Convert the version string to internal code format
    atascii_to_internal(version, 40);

    // Initialize the frame buffer
    // Initialize the frame buffer
    saveCurrentGraphicsState();

    // Detect VBXE
    detect_VBXE();

    // Initialize the settings.  Set defaults if no saved settings are found.
    get_settings();

    // Stop the attract mode
    ORIG_VBII_SAVE = (ushort)OS.vvblki;
    add_attract_disable_vbi();
}

void sys_process_args(char* argv[])
{
    // Not used on Atari currently as args are handled in main.c generic logic
}

void sys_disable_screensaver(void)
{
    add_attract_disable_vbi();
}

void sys_enable_screensaver(void)
{
    remove_attract_disable_vbi();
}

void sys_sound_enable(bool enable)
{
    if (enable)
        OS.soundr = 3;
    else
        OS.soundr = 0;
}

void sys_flush_input(void)
{
    while(kbhit()) cgetc();
}

char sys_get_key(void)
{
    return cgetc();
}

bool sys_key_pressed(void)
{
    return kbhit();
}

void sys_cleanup(void)
{
    setGraphicsMode(GRAPHICS_0);
    restoreGraphicsState();
}
