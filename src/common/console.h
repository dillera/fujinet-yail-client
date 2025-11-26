// Copyright (C) 2021 Brad Colbert

#ifndef CONSOLE_H
#define CONSOLE_H

#define NUM_TOKENS 32

void show_console(void);
void hide_console(void);
void start_console(char first_char);
void console_update(void);
void reset_console(void);
void fix_chars(char*);

#endif // CONSOLE_H