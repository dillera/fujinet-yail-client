#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"
#include <stdbool.h>

// System initialization
void sys_init(void);

// System specific argument processing (if any)
void sys_process_args(char* argv[]);

// Screensaver/Attract mode control
void sys_disable_screensaver(void);
void sys_enable_screensaver(void);

// Sound control
void sys_sound_enable(bool enable);

// Console/IO
void sys_flush_input(void);
char sys_get_key(void);
bool sys_key_pressed(void);

// System cleanup
void sys_cleanup(void);

// Platform specific constants or globals
extern byte buff[];

#endif
