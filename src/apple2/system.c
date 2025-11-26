#include "system.h"
#include "settings.h"
#include <conio.h>
#include <stdbool.h>

extern char version[];

void sys_init(void)
{
    // get_settings();
}

void sys_process_args(char* argv[]) {}

void sys_disable_screensaver(void) {}
void sys_enable_screensaver(void) {}

void sys_sound_enable(bool enable) {}

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
    clrscr();
}
